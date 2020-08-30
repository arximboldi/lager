
.. _action:
.. _actions:

Actions
=======

Actions are values that determine external events and user
interactions that cause the :ref:`model` to change, as expressed in
`business logic`_ language.

.. _business logic: https://en.wikipedia.org/wiki/Business_logic

Actions are values
------------------

It is important to note that **actions are values**.  They are a
declarative description of what may happen, not the happening in
itself. In the :ref:`Model` section we discuss value-semantic design
in detail.  All those concerns also apply to the design of actions.

Type-safe actions
-----------------

There are many ways you can define actions.  Normally, in an
application there are different *kinds* of actions.  Consider a
typical CRUD_ application, like the canonical `Todo List`_ example.
To let the type system help us deal with the different actions, we may
define the actions as different types whose instances carry all the
information needed to perform the operation:

.. _crud: https://en.wikipedia.org/wiki/Create%2C_read%2C_update_and_delete
.. _todo list: http://todomvc.com/examples/elm/

.. _todo-actions:
.. code-block:: c++

   struct add_todo { std::string content; };
   struct remove_todo { std::size_t index; };
   struct toggle_todo { std::size_t index; };

These actions may act on a :ref:`model` like this:

.. code-block:: c++

   struct todo
   {
       immer::box<std::string> content;
       bool completed = false;
   };

   struct todos_model
   {
       immer::flex_vector<todo> todos;
   };

.. note:: We use here the containers types from the Immer_ library of
          immutable data-structures instead of those of the standard
          library.  These are discussed in the :ref:`performance
          <performance>` section.

.. _immer: https://github.com/arximboldi/immer

We can now implement a :ref:`reducer<reducers>` for each of the operations as
overloads of an ``update_todos()`` function:

.. code-block:: c++

   todos_model update_todos(todos_model m, add_todo a)
   {
       m.todos = std::move(m.todos).push_back({a.content, false});
       return m;
   }

   todos_model update_todos(todos_model m, remove_todo a)
   {
       m.todos = std::move(m.todos).erase(a.index);
       return m;
   }

   todos_model update_todos(todos_model m, toggle_todo a)
   {
       m.todos = std::move(m.todos).update(a.index, [] (auto t) {
           t.completed = !t.completed;
           return t;
       });
       return m;
   }

Once we have this family of actions and their corresponding reducers,
we can use `std::variant`_ and `std::visit`_ to combine them into one
single type and function, that we can use when building the
:cpp:class:`lager::store`:

.. _std::variant: https://en.cppreference.com/w/cpp/utility/variant
.. _std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit

.. code-block:: c++

   using todo_action = std::variant<
       add_action,
       remove_action,
       toggle_action
   >;

   todos_model update(todos_model m, todos_action a)
   {
       return std::visit([&] (auto a) { return update_todos(m, a); }, a);
   }

This approach of using ``std::variant`` to combine strongly typed
actions has multiple advantages:

- Actions are simple :ref:`value types <value-semantics>`. It is easy
  to add serialization and other inspection mechanisms.

- We can use `function overloading`_ to distinguish different types of
  actions.

- When `pattern matching`_ the combined action type the compiler will
  complain if we fail to cover some cases.

- It works well when composing components hierarchically. We
  will discuss this in the :ref:`modularity` section.

.. _pattern matching: https://en.wikipedia.org/wiki/Pattern_matching
.. _function overloading: http://www.cplusplus.com/doc/tutorial/functions2/

.. tip:: You do not need to write one separate *reducer* function per
         action type, like we did in this section.  In the
         :ref:`architecture` section we showed how to use
         :cpp:class:`lager::visitor` to :ref:`pattern match the action
         variant using lambdas<pattern-match-example>`.  This lowers
         the amount of boiler-plate required for small reducers.
         There are other libraries like Scelta_, Atria_ or
         `Boost.Hof`_ that are convenient when dealing with variants.

.. _scelta: https://github.com/SuperV1234/scelta
.. _atria: https://github.com/Ableton/atria
.. _Boost.Hof: https://www.boost.org/doc/libs/release/libs/hof/doc/html/doc/index.html

Alternative schemes
-------------------

While type-safe action is the preferred way of defining actions, and
the one used most often in this document, it is important to note that
you can freely define actions however you want, and there are
situations where other alternative designs might be better.

Stringly typed actions
~~~~~~~~~~~~~~~~~~~~~~

Instead of using types and variants, you could use ``enum`` and
``switch``/``case`` to identify the different kinds of actions.  You
still need to somehow access the different kinds of arguments to the
actions, for which you may need to resort to ``union`` or mechanism,
which is unsafe while bringing no additional advantages.

In Redux_, because of JavaScript, they often use instead `stringly
typed`_ actions.  This is rarely advantageous in C++, but there are
situations where you may want to do so, for instance, when
implementing a command line or configurable shortcuts.  When doing so,
it is still useful to have a type safe core set of actions, and to
implement the stringly typed ones in terms of them.  For example, we
can extend the :ref:`todo actions<todo-actions>` defined above by
adding a string-based action type and a corresponding reducer:

.. _redux: https://redux.js.org/basics/actions
.. _stringly typed: http://wiki.c2.com/?StringlyTyped

.. _intent-example:
.. code-block:: c++

   struct todos_command
   {
       std::string command;
       std::string argument;
   };

   todos_model update(todos_model m, todos_command c)
   {
       static const auto command_actions =
         std::map<std::string, std::function<todos_action(std::string)>>{
           "add",    [] (auto arg) { return add_todo{arg}; },
           "remove", [] (auto arg) { return remove_todo{std::stoi(arg)}; },
           "toggle", [] (auto arg) { return toggle_todo{std::stoi(arg)}; },
       };
       auto it = command_actions.find(c.command);
       if (it == command_actions.end())
           return m;
       else
           return update(m, it->second(c.argument));
   }

This can also be considered an alternative way of implementing an
``intent()`` function, as :ref:`suggested in the Architecture
section<intent>`.

Function actions
~~~~~~~~~~~~~~~~

Some people consider that separating action types and reducers is a
form of boiler plate.  As such, they are tempted to combine the two.
For example, the :ref:`todos actions<todo-actions>` and reducer could
be rewriten as:

.. code-block:: c++

   using todos_action = std::function<todos_model(todos_model)>;

   todos_action add_todo(std::string content)
   {
       return [=] (auto m) {
           m.todos = std::move(m.todos).push_back({a.content, false});
           return m;
       };
   };

   todos_action remove_todo(std::size_t index)
   {
       return [=] (auto m) {
           m.todos = std::move(m.todos).erase(index);
           return m;
       };
   }

   todos_model toggle_todo(std::size_t index)
   {
       return [=] (auto m) {
           m.todos = std::move(m.todos).update(a.index, [] (auto t) {
               t.completed = !t.completed;
               return t;
           });
           return m;
       };
   }

   todos_model update(todos_model model, todos_action action)
   {
      return action(model);
   }

This approach is, in general, not recommended.  While functions that
do not capture references are, in fact, values, they are so only in a
rather weak sense.  They are opaque, imposing several limitations:

- We can not properly define equality of functions.
- The arguments of the action, once captured, can not be inspected.
- They can not be serialized.
