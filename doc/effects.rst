
.. _effects:

Effects
=======

Effects allow :ref:`reducers` to request the execution of code that
would break the :ref:`purity<purity>` of the reducer .  In this way,
we can describe I/O and other interactions directly from the reducer,
without directly executing them.

Writing effects
---------------

The type :ref:`lager::effect<effect-type>` is defined in the library as:

.. code-block:: c++

   template <typename Action, typename Deps = lager::deps<>>
   using effect = std::function<void(const context<Action, Deps>&)>;

This is, an *effect* is just a procedure that takes some
:cpp:class:`lager::context` as an argument.

The *context* is parametrized over an ``Action`` type.  This allows
the effect to deliver new actions by using the
:cpp:func:`lager::context::dispatch` method.  For example, if you
tried to write a file in an effect, you probably want to dispatch
actions to inform the system about whether the operation succeeded or
failed.  The context also has an optional ``Deps`` parameter, which
allows the effect to use a :ref:`dependency
injection<depenecy-injection>` mechanism to get access to *services*
that it might need to perform the side effects.

To produce an effect, you can just return it from the reducer.  In the
previous section we learnt that :ref:`reducers` are functions with a
signature of the form:

.. code-block:: c++

   auto update(Model, Action) -> Model;

However, a reducer may alternatively be a function of the form:

.. code-block:: c++

   auto update(Model, Action) -> std::pair<Model, lager::effect<Action>>;

The :cpp:class:`lager::store` automatically detects at compile time
whether the given reducer returns effects. If it does, it schedules
the excution of the effects in the main loop, directly after the
evaluation of the update function.

.. note::

   Technically the effectful reducer does not need return a
   ``std::pair``, but anything that defines ``std::get`` and returns a
   model at index ``0`` and an effect at index ``1``.  This means that
   you can use an ``std::tuple`` instead, for example.  Also, the
   effect does not need to match the type exactly, it just needs to be
   convertible to it.

.. _intent-effect-example:
A minimalist example
--------------------

The simplest useful effect is an effect that simply delivers another
action.

In architectural introduction we introduced the notion of
:ref:`intent`, a function that maps actions expressed in UI language
to actions expressed in application logic language.  Later, we showed
:ref:`an example <intent-example>` of a reducer that expresses
*intent* by directly calling a reducer with the derived action.

However, sometimes one might prefer to have the reducer return to the
main loop before delivering the new action.  This allows Lager to see
the newly generated action.  This means that this action can be
inspected in the :ref:`time travelling debugger<time-travel>` and
other tools.

The :ref:`previous example <intent-example>` can thus be rewritten
using a reducer that, when it receives a UI logic action, communicates
intent by returning an effect that delivers an application logic
action.

.. code-block:: c++

   #include <lager/context.hpp>

   using action = std::variant<todos_action, todos_command>;

   using todos_result = std::pair<todos_model, lager::effect<action>>;

   todos_result update(todos_model m, action a)
   {
       return std::visit(lager::visitor{
           [] (const todos_action& a) -> todos_result {
              return {update(m, a), lager::noop};
           },
           [] (const todos_command& c) -> todos_result {
               static const auto command_actions =
                 std::map<std::string, std::function<todos_action(std::string)>>{
                   "add",    [] (auto arg) { return add_todo{arg}; },
                   "remove", [] (auto arg) { return remove_todo{std::stoi(arg)}; },
                   "toggle", [] (auto arg) { return toggle_todo{std::stoi(arg)}; },
               };
               auto it = command_actions.find(c.command);
               if (it == command_actions.end())
                   return {m, lager::noop};
               else {
                   auto new_action = it->second(c.argument);
                   return {m, [] (auto&& ctx) {
                       ctx.dispatch(new_action);
                   }};
               }
           },
       }, a);
   }

Note how we use a ``std::variant`` to combine the *business logic*
action type (``todo_action``) with the *UI logic* action
(``todos_command``).  When we receive a business logic action, we just
forward to its reducer.  When we receive a UI level action, we figure
out whether this action should result in a business logic action,
and if so, we deliver it via an effect.

.. note::

   We used :ref:`lager::noop<noop>` as an *empty* effect in the paths
   that do not require one.  It can be more efficient than using ``[]
   (auto&&) {}`` because Lager detects it, completely bypassing the
   evaluation of the effect.

.. _dependency-injection:
Dependency passing
------------------

Oftentimes, the effect will need to access some *service* in order to
do its deed.  For example, when performing asynchronous IO it may need
to access some `boost::asio::io_context`_. Or maybe the effect should
forward to some other service of your own that encapsulates the I/O
logic.  By definition, these types are referential, so you can not put
them in the model or the action that is passed to the reducer that
generates the effect.

.. _boost\:\:asio\:\:io_context: https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/io_context.html

Instead, the framework can deliver these services for you by declaring
the dependency in the effect signature using the
:cpp:class:`lager::deps` type.  For example, an effect that wants a
reference to a ``boost::asio::io_context`` can be declared like this:

.. code-block:: c++

   lager::effect<action, lager::deps<boost::asio::io_context&>> eff =
       [] (auto&& ctx) {
           auto& io = get<boost::asio::io_context>(ctx);
           io.post([] { ... }); // for example
       };

If the *reducer* returns such an effect, the store will pass the
dependencies to it, embeded in the context.  The ``get()`` function is
used to access the dependencies inside the effect.  For this to work,
we need to provide the dependencies to the store by using
:cpp:func:`lager::with_deps` as an extra argument to the
:cpp:func:`lager::make_store` factory:

.. code-block:: c++

   auto io    = boost::asio::io_context{};
   auto store = lager::make_store(
       // ...
       lager::with_deps(std::ref(io)));

If you fail to provide all the dependencies required for the effects,
there will be a compilation error.

.. warning:: Dependencies are passed by value by default. Use
             ``std::ref`` to mark the dependencies that shall be
             passed by reference.

Identifying dependencies
~~~~~~~~~~~~~~~~~~~~~~~~

In the previous example, there is ony one instance of
``boost::asio::io_context`` that is passed to all the effects that are
evaluated within the Lager context.  This is a good replacement of the
`singleton design pattern`_: there is a single instance, but there is
low physical coupling and you can still replace the instance in a
different context, particularly in unit tests.

.. _singleton design pattern: https://en.wikipedia.org/wiki/Singleton_pattern

However, we often will need to have multiple instances of a type
provided to different subsystems.  We can declare type tags to
differentiate these instances using these types as keys:

.. code-block:: c++

   struct foo_dep {};
   struct bar_dep {};

   lager::effect<foo_action,
                 lager::deps<lager::dep::key<foo_dep,
                                             boost::asio::io_context&>>
       foo_effect = [] (auto&& ctx) {
           auto& io = get<foo_dep>(ctx);
           // ...
       };

   lager::effect<bar_action,
                 lager::deps<lager::dep::key<bar_dep,
                                             boost::asio::io_context&>>
       bar_effect = [] (auto&& ctx) {
           auto& io = get<bar_dep>(ctx);
           // ...
       };

Now we can provide two separate ``io_context`` instances for the two
subsystems:

.. code-block:: c++

   using boost::asio::io_context;
   auto foo_io = io_context{};
   auto bar_io = io_context{};
   auto store  = lager::make_store(
       // ...
       lager::with_deps(
           lager::dep::as<lager::dep::key<foo_dep, io_context&>>(foo_io),
           lager::dep::as<lager::dep::key<bar_dep, io_context&>>(bar_io)));

Using this technique, the names of the dependencies are still static.
This makes the mechanism very efficient and ensures that dependency
mismatches are cought at compile time.

If the number of instances of a dependency is determined dynamically,
you will need to define a kind of *manager* for these instances and
provide this manager as a service instead.  This naturally involves
defining a :ref:`identity` scheme for these dependencies, such that
the effects that required them can refer to them.

Other dependency specs
~~~~~~~~~~~~~~~~~~~~~~

The type :cpp:type:`lager::dep::key` is a *dependency specification*,
used to describe how the dependency is required.  There are other
specifications, and they can be combined:

- :cpp:type:`lager::dep::opt` is used to notate **optional** dependencies.
  There won't be a compilation error if a depedency is missing when a
  module requests it as optional.  Instead, get may throw an
  exception.  You can check if the dependency is provided using
  :cpp:func:`lager::has`.

- :cpp:type:`lager::dep::fn` is used to notate **lazy** dependencies.
  These may not be available directly when building the store, but are
  instead to be requested lazily through a provided function.
