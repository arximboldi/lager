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
   auto cursor_type<model_type>::zoom(lens<model_type, model_part>)
       -> maybe_other_cursor_type<model_part>;
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
       });
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
           },
           [&](add_room_action a) {
               h.rooms = h.rooms.set(a.id, a.r);
           }
       });
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

   // The result is lager::reader<room>
   auto kitchen_cursor = store
       .zoom(lager::lenses::attr(&house::rooms))
       .zoom(lager::lenses::at("kitchen"))
       // maybe you want to use some other
       // approach to deal with this std::optional
       .zoom(lager::lenses::or_default());

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
           , light_on(r.zoom(lager::lenses::attr(&room::light_on)))
           , l(light_state(light_on.get()))
       {
           lager::watch(light_on, [&] (bool on) {
               l.set_text(light_state(on));
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
