
.. _effects:

Effects
=======

Effects allow :ref:`reducers` to request the execution of code that
would break the :ref:`purity<purity>` of the reducer .  In this way,
we can describe I/O and other interactions directly from the reducer.

Writing effects
---------------

The type :ref:`lager::effect<effect-type>` is defined in the library as:

.. code-block:: c++

   template <typename Action, typename Deps = lager::deps<>>
   using effect = std::function<void(const context<Action, Deps>&)>;

This is, an *effect* is just a procedure that takes some
:cpp:class:`lager::context` as an argument.

The *context* is parametrized over an ``Action``.  This allows the
effect to deliver new actions by using the
:cpp:func:`lager::context::dispatch` method.  For example, if you
tried to write a file in an effect, you probably want to inform the
reducer back of whether this succeeded or failed by delivering
actions.  It also has an optional ``Deps`` parameter, which allows the
effect to use a :ref:`dependency injection<depenecy-injection>`
mechanism to get access to *services* that it might need to perform
the side effects.

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

   Technically the effect does not need return a ``std::pair``, but
   anything that defines ``std::get`` and returns a model at index
   ``0`` and an effect at index ``1``.  This means that you can use an
   ``std::tuple`` instead, for example.  Also, the effect does not
   need to match the type exactly, it just needs to be convertible to
   it.

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
using effects:

.. code-block:: c++

   using action = std::variant<todos_action, todos_command>

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
action type (``todo_action``) with the UI level action
(``todos_command``).  When we receive a business logic action, we just
forward to its reducer.  When we receive a UI level action, we figure
out if this action results should result in a business logic action,
and if so, we deliver it via an effect.

.. note::

   We used :ref:`lager::noop<noop>` as an *empty* effect in the paths
   that do not require one.  It can be more efficient than using ``[]
   (auto&&) {}`` because it completely bypasses the evaluation of the
   effect.

.. _dependency-injection:
Dependency injection
--------------------
