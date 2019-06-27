
Design overview
===============

Unidirectional-data flow
------------------------

.. image:: _static/architecture.svg
   :width: 80%
   :align: center

This diagram represents the architecture of a Lager based application.
It is somewhat similar to common representations of the `Model View
Controller`_, however, there are some fundamental differences:

* The *boxes* do not represent stateful objects with their own identity,
  but **value-types** with deep-copy and logical equality semantics.

* The *arrows* do not represent references or aggregation, but
  **pure functions** that only transform data.

There are only two stateful methods, the :ref:`effects`, and the
:cpp:func:`lager::store::dispatch`.

.. _model view controller:
   https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93controller

Basic example
-------------

Let's make a very basic example. For now, it is going to be an
interactive application where you can use commands to manipulate the
counter.

We follow a `value-oriented design`_.  we will start by looking at the
data we need, and then look at the transformations that data-required.

.. admonition:: Diferences with Data-Oriented Design.

   In the last years, we have seen Data-Oriented Design.

Model
~~~~~

The :ref:`model` is a value type that contains a snapshot of the state
of the application.  For our example application, something like this
suffices.

.. code-block:: c++

   struct model
   {
       int value = 0;
   };

Actually, we could just have aliased the ``int``, but normally we will
have more complicated models with multiple pieces of data bound
together in a ``struct``.

Actions
~~~~~~~

The :ref:`action` is a value type that represents something that the
user did in the application.  Normally, the user can do multiple
things.  We can use different *structs* to represent the various
things that the user can do, then bind them together using a
`std::variant`_

.. code-block:: c++

   struct increment_action {};
   struct decrement_action {};
   struct reset_action { int new_value = 0; };

   using action = std::variant<increment_action, decrement_action, reset_action>;


.. _std::variant: https://en.cppreference.com/w/cpp/utility/variant
.. _business logic: https://en.wikipedia.org/wiki/Business_logic
