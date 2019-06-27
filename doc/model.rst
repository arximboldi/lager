
.. _model:

Model
=====

The data model design is key in a Lager application.  The good news
is: changing the design and shape of the data-model in a
value-oriented world tends to be easy.  However, for this to be the
case, it is important that we follow some principles strictly.

Value semantics
---------------

In the previous section we emphathised that the data-model **must be a
value type**.  But what is a `value type`_ really?

In a purely `functional programming`_ language, like Haskell,
everything you name is a *value*.  This is, entities that, like
mathematical objects, are immutable and have some sense of equality.

In a multi-paradigm language like C++, you have both *value types* and
*reference types*.  Intances of value types define its meaning through
the values that they represent, that is why we say, they have *value
semantics*.  Each instance of a value type has its own copy of a
representation of a value, and can be operated independently.  Most of
the time, we do not care about the identity of the instance, but about
the content, the value, which can be *equal* between instances.
*Reference types*, however, reference other mutable objects.  The
identity of these objects is crucial.  We can observe changes to the
refered objects through the referees.  This means that we have to be
particularly careful when dealing with reference types.

The canonical reference types in C++ are references (``&``) and
pointers (``*``).  There are many other reference types.  For example,
iterators of a container are references that point into the
container.  If we change the container, we can observe the changes
through the iterator.  Sometimes even, the iterator may become invalid.

In C++ value types are not necessarily immutable.  For example, I can
mutate a `std::vector`_ through its ``push_back()`` method.  Or I can
modify a ``struct`` by assigning to its members.  Instead, *they are
value types because each instance of the type is independent and is a
attributed meaning through the values it represent.* This **principle
of independence** is highlighted by this code snippet:

.. code-block:: c++

   auto a = std::vector<int>{};
   auto b = a;
   foo(b);
   assert(a.empty());

Here we have an instance of an empty ``std::vector`` named ``a``, and
copy it into another instance ``b``.  Then we call ``foo()``.  Then we
make an assertion about the state of ``a``.  See, I do not need to
know anything about what ``foo()`` does to reason about the state of
``a``. It could have taken ``b`` by reference and changed it.  Maybe
it even did so from another thread.  In any case, we can be sure that
our assertion about ``a`` holds, because ``b`` made an independent
copy of it.  This frees our mind, allowing us to reason locally about
the state of the program.  This is fundamental to write correct
concurrent programs: we just have to make sure each thread works on
their local "copy" of the data.

.. _value type: https://en.wikipedia.org/wiki/Value_type_and_reference_type
.. _std::vector: https://en.cppreference.com/w/cpp/container/vector
.. _functional programming: https://en.wikipedia.org/wiki/Functional_programming

.. admonition:: Spotting reference types
   :class: warning

   In order to avoid them, it is important to be able to cleary
   identify *reference types*.  They are sneaky.  I said that an
   ``std::vector<T>`` is a value type: this is true only as long as
   ``T`` is also a value type.  Same applies to ``struct``, it is not
   valuey as soon as a member is not.

   Also, there are some C++ types that behave a bit like values, they
   seem to be copyable at least---but their copies reference external
   entities that mutate, and these changes can be observed through all
   these shallow copies!  Here are some examples of such types that
   shall be avoided in our data-model:

   * A **pointer**.  A pointer has a value, which is the memory location
     of an object. But we can also use this pointer value to change
     and observe changes in this memory location.

   * An **iterator** into a container, as mentioned above.

   * A **file handle**.  The file changes independently and we can use the
     handle to observe or even trigger these changes.  The file can even
     change outside of our program.

   * Most *resource handles* actually. Ids of OpenGL buffers, Posix
     process handles, lots of these things look like an ``int``, but
     they are not, they are a reference to a mutable entity.

   * *Function objects* that capture references, particularly lambdas
     with ``&``-captures.

.. admonition:: Are pointers always bad?

   Not always. For instance, the *implementation* of ``std::vector``
   uses pointers internally.  But it defines ``operator=`` to ensure
   the independence principle.  Also, Sean Parent shows in his famous
   talk `"Inheritance is the base class of evil"`_ that given a value
   type ``T``, a forest of ``std::shared_ptr<const T>`` is
   effectively, a value type.  However, the ``immer::box`` type from
   the Immer_ library serves the same purpose with safer and more
   convenient interface.  In the :ref:`performance` section we will
   conver the power immutability in more detail.

   In general, the application level data model must never use
   pointers.  It can however generic container types that encapsulate
   the pointers properly deal with the intricacies of properly
   defining ``operator=`` as to ensure the independence principle.

.. _"Inheritance is the base class of evil": https://www.youtube.com/watch?v=bIhUE5uUFOA
.. _immer: https://github.com/arximboldi/immer

.. _identity:
Identity
--------

whaaat

.. _normalization:

Normalization
-------------

hola la la

.. _performance:
Performance
-----------
