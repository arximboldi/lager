.. _cursors:

Cursors
=======

Built upon :ref:`lenses`, cursors are a great way to bridge
value-oriented designs and object-oriented systems. It also allows
modularity.

.. _types-of-cursors:

Types of cursors
----------------

Cursors are wrappers for :ref:`models <model>` where you can get the current
version of a model, watch it for changes, zoom into other cursors
and (for some types of cursors) replace the model with another version:

.. code-block:: c++

   // Call signatures of cursors
   model_type cursor_type<model_type>::get();
   void lager::watch(cursor_type<model_type> cursor,
                     std::function<void(model_type)> callback);
   // for some types only
   void cursor_type<model_type>::set(model_type new_model);

Lager provides four types of cursors:

* ``lager::reader`` provides ``get()`` and ``watch()`` functionalities.

* ``lager::cursor`` provides all the functionalities ``lager::reader``
  provides, and also ``set()`` functionality.

* ``lager::state`` provides all the functionalities ``lager::cursor``
  provides. It also serves as the single source of truth by directly
  containing the model.

* ``lager::store`` provides all the functionalities ``lager::reader``
  provides. It also serves as the single source of truth by directly
  containing the model. Unlike ``lager::cursor`` and ``lager::state``,
  it makes changes to models by dispatching :ref:`actions`, instead of
  the ``set()`` function.

``lager::reader`` and ``lager::cursor`` zoom into themselves, while
``lager::state`` zooms into ``lager::cursor`` and ``lager::store``
zooms into ``lager::reader``.

Toplevel cursors
----------------

Toplevel cursors are ``lager::state`` or ``lager::store``. They are
where all other cursors come from. To create a toplevel cursor, use:

.. code-block:: c++

   #include <lager/state.hpp>

   // state defaults to transactional_tag, which means you
   // need to call `lager::commit(state);` if you want
   // the values set in one cursor got propagated to
   // other cursors.
   auto state = lager::make_state(model);
   // You will not need to call `lager::commit(state);`
   // if you use automatic_tag{};
   // the values will automatically be propagated to
   // other cursors.
   auto state = lager::make_state(model, lager::automatic_tag{});

or:

.. code-block:: c++

   #include <lager/store.hpp>

   auto store = lager::make_store<action>(
       model, update, event_loop, enhancers...);

Zooming with lenses
-------------------

One can use ``zoom()`` method to zoom a cursor into another:

.. code-block:: c++

   auto cursor_type<model_type>::zoom(lens<model_type, model_part>)
       -> maybe_other_cursor_type<model_part>;

For example:

.. code-block:: c++

   #include <lager/state.hpp>

   using map_t = immer::map<std::string, int>;
   using arr_t = immer::array<std::string>;
   struct whole {
       part p;
       map_t m;
       arr_t a;
   };

   lager::state<whole> state = lager::make_state(whole{});
   lager::cursor<part> part_cursor =
       state.zoom(lager::lenses::attr(&whole::p));
   lager::cursor<map_t> map_cursor =
       state.zoom(lager::lenses::attr(&whole::m));
   lager::cursor<int> int_cursor =
       map_cursor.zoom(lager::lenses::at("foo"))
       .zoom(lager::lenses::or_default);
   lager::cursor<std::string> str_cursor =
       state.zoom(lager::lenses::attr(&whole::a))
       .zoom(lager::lenses::at(0))
       .zoom(lager::lenses::value_or("no value"));

For convenience, one can also use the ``operator[]``, which
takes a lens, key (index) or pointer to attribute. The latter
two will be converted into a lens using ``lager::lenses::at``
and ``lager::lenses::attr`` automatically. The example above
can also be written as:

.. code-block:: c++

   lager::cursor<part> part_cursor =
       state[&whole::p];
   lager::cursor<map_t> map_cursor =
       state[&whole::m];
   lager::cursor<int> int_cursor =
       map_cursor["foo"][lager::lenses::or_default];
   lager::cursor<std::string> str_cursor =
       state[&whole::a][0][lager::lenses::value_or("no value")];

Transformations
---------------

The ``xform()`` function is another way to transform the cursor.
For read-only cursors (``lager::store`` and ``lager::reader``),
it takes one transducer; for read-write cursors (``lager::state``
and ``lager::cursor``), it can take two to transform into another
read-write cursor, or take one to transform into a read-only
cursor.

.. code-block:: c++

   lager::reader<std::string> str = ...;
   // One-way transformation for read-only cursors
   lager::reader<int> str_length = str.xform(
       zug::map([](std::string x) { return x.size(); }));

   lager::cursor<std::string> str = ...;

   // Two-way transformation for read-write cursors
   lager::cursor<int> num = str.xform(
       zug::map([](std::string x) { return std::stoi(x); }),
       zug::map([](int x) { return std::to_string(x); })
   );

   // One-way transformation to make a read-only cursor
   lager::reader<int> num2 = num.xform(
       zug::map([](int x) { return 2*x; }));

   str.set("123");
   // You need `lager::commit(state);`
   // if you use transactional_tag
   std::cout << num.get() << std::endl; // 123
   num.set(42);
   std::cout << str.get() << std::endl; // 42
   std::cout << num2.get() << std::endl; // 84

Combinations
------------

You can combine more than one cursors into one using ``with()``.
The resulted cursor will be of a ``std::tuple`` containing
all the value types in the original cursors:

.. code-block:: c++

   #include <lager/with.hpp>

   lager::cursor<int> num = ...;
   lager::cursor<std::string> str = ...;
   lager::cursor<std::tuple<int, std::string>> dual =
       lager::with(num, str);

   // If any of the cursors passed into with() are read-only,
   // it will result in a read-only cursor.
   lager::reader<std::string> str_ro = ...;
   lager::reader<std::tuple<int, std::string>> dual_ro =
       lager::with(num, str_ro);

.. _using-cursors:

Using cursors
-------------

We will use a minimal example to show how cursors work. Suppose one
wants to represent a house using the following models, actions and
reducers:

.. code-block:: c++

   #include <lager/util.hpp>

   struct room
   {
       bool light_on;
   };

   struct toggle_light_action {};
   using room_action = std::variant<toggle_light_action>;

   room update_room(room r, room_action a)
   {
       return std::visit(lager::visitor{
           [=](toggle_light_action) {
               return room{ ! r.light_on };
           }
       }, a);
   }

   struct house
   {
       immer::map<std::string, room> rooms;
   };

   struct change_room_action
   {
       std::string id;
       room_action a;
   };
   struct add_room_action
   {
       std::string id;
       room r;
   };
   using house_action = std::variant<change_room_action,
                                     add_room_action>;

   house update_house(house h, house_action a)
   {
       return std::visit(lager::visitor{
           [&](change_room_action a) {
               auto old_room = h.rooms[a.id];
               auto new_room = update_room(old_room, a.a);
               // For simplicity we do not add move semantics
               // here, but you should do in your own program
               h.rooms = h.rooms.set(a.id, new_room);
               return h;
           },
           [&](add_room_action a) {
               h.rooms = h.rooms.set(a.id, a.r);
               return h;
           }
       }, a);
   }

Create the single source of truth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As discussed above, we have two choices for our single source of
truth: ``lager::state`` or ``lager::store``. If you are refactoring
old code, it may be a good choice to use ``lager::state`` because
it allows you to gradually lift the state up without rewriting
everything at once. If you are developing new software, it may be
worthy to to use ``lager::store`` to benefit from the use of
actions.

Here, we will use ``lager::store`` as an example.

.. code-block:: c++

   #include <lager/store.hpp>
   #include <lager/event_loop/manual.hpp>

   // Make an initial model
   house initial_house;
   initial_house.rooms = initial_house.rooms
       .set("kitchen", room{false});
   initial_house.rooms = initial_house.rooms
       .set("bedroom", room{true});

   auto store = lager::make_store<house_action>(
       initial_house,
       &update_house,
       // Be sure to use a suitable event loop
       // that integrates into the rest of your program
       lager::with_manual_event_loop{});


Zooming the cursors
~~~~~~~~~~~~~~~~~~~

Suppose we want to access and watch the state of the kitchen.
We can use the ``zoom()`` method to obtain a cursor just
for that:

.. code-block:: c++

   #include <lager/lenses/at.hpp>
   #include <lager/lenses/attr.hpp>
   #include <lager/lenses/optional.hpp>

   lager::reader<room> kitchen_cursor = store
       .zoom(lager::lenses::attr(&house::rooms))
       .zoom(lager::lenses::at("kitchen"))
       // maybe you want to use some other
       // approach to deal with this std::optional
       .zoom(lager::lenses::or_default);

   // You can now query for the state:
   auto kitchen = kitchen_cursor.get();

   auto kitchen_light_on = kitchen.light_on;

Using cursors in object-oriented views
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Suppose we want to display our room in an object-oriented
GUI library, we can make the widget receive a cursor to the
room model and watch it for changes:

.. code-block:: c++

   class room_component : public widget
   {
       lager::reader<room> r;
       lager::reader<bool> light_on;
       label l;

       static std::string light_state(bool on) {
           return on ? "light is on" : "light is off";
       }
   public:
       room_component(lager::reader<room> r, widget *parent = 0)
           : widget(parent)
           , r(r)
           , light_on(r[&room::light_on])
           , l(light_state(light_on.get()))
       {
           lager::watch(light_on, [&] (bool on) {
               l.set_text(light_state(on));
           });
       }
   };

Dispatching actions
~~~~~~~~~~~~~~~~~~~

Of course, we do not want the GUI to only display
the model. Instead, we would like to allow it make changes
to our model. Here, since we are using ``lager::store`` as
our single source of truth, we benefit from making changes
through actions.

We dispatch actions through contexts. Here, ``lager::store``
is a context. We may directly dispatch actions via the store:

.. code-block:: c++

   store.dispatch(change_room_action{"kitchen",
                  toogle_light_action{}});

But for the ``room_component`` we have here, it may not be a
great idea, because it breaks modularity. If we were to
dispatch an action via ``store``, the room component will
need to know the room's id. In other words, it has to know
something about the house, rather than only know about
the room itself. We would like to have a context that can
dispatch a ``room_action``, instead of a ``house_action``:

.. code-block:: c++

   ctx.dispatch(toogle_light_action{}); // what should ctx be?

Fortunately, lager provides a context conversion constructor
that can be used here, and the only thing we would like to do
is to provide a conversion function that converts a
``room_action`` into a ``house_action``:

.. code-block:: c++

   std::string room_id = "kitchen";
   auto ctx = lager::context<room_action>(
       store,
       [=](room_action a) -> house_action {
           return change_room_action{ room_id, a };
       });

And now we can add a toggle button to our room component
to control the light:

.. code-block:: c++

   class room_component : public widget
   {
       lager::reader<room> r;
       lager::reader<bool> light_on;
       lager::context<room_action> ctx;
       label l;
       button b;

       static std::string light_state(bool on) {
           return on ? "light is on" : "light is off";
       }
   public:
       room_component(lager::reader<room> r,
                      lager::context<room_action> ctx,
                      widget *parent = 0)
           : widget(parent)
           , r(r)
           , light_on(r.zoom(lager::lenses::attr(&room::light_on)))
           , ctx(ctx)
           , l(light_state(light_on.get()))
           , b("Toogle light")
       {
           lager::watch(light_on, [&](bool on) {
               l.set_text(light_state(on));
           });

           b.clicked.connect([ctx=this->ctx]() {
               ctx.dispatch(toogle_light_action{});
           });
       }
   };


.. _additional-resources:

Additional resources
--------------------

To learn more about cursors, you can watch the **C++ Russia 2019 Talk**:
`Squaring the circle: value oriented design in an object oriented system
<https://www.youtube.com/watch?v=e2-FRFEx8CA>`_ (`slides`_).

.. _slides: https://sinusoid.es/talks/cpprussia19-piter
