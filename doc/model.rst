
.. _model:

Model
=====

The data model design is key in a Lager application.  The good news
is: changing the design and shape of the data-model in a
value-oriented world tends to be easy.  However, for this to be the
case, it is important that we follow some principles strictly.

.. _value-semantics:
.. _independence-principle:
Value semantics
---------------

In the previous section we emphasised that the data-model **must be a
value type**.  But what is a `value type`_ really?

In a purely `functional programming`_ language, like Haskell,
everything you name is a *value*.  This is, all entities are like
mathematical objects: they are immutable and have some definition of
equivalence.

In a multi-paradigm language like C++, you have both *value types* and
*reference types*.  Intances of value types define its meaning through
the values that they represent, that is why we say, they have *value
semantics*.  Each instance of a value type has its own copy of a
representation of a value, on which we can operate independently.
Most of the time, we do not care about the identity of an instance,
but about the content, the value, which can be *equal* between
instances.  *Reference types*, however, reference other mutable
objects.  The identity of these objects is crucial.  We can observe
changes to the referred objects through the references.  This means
that we have to be particularly careful when dealing with reference
types.

The canonical reference types in C++ are references (``&``) and
pointers (``*``).  There are many other reference types.  For example,
iterators of a container are references that point into the
container.  If we change the container, we can observe the changes
through the iterator.  Sometimes even, the iterator may become invalid.

In C++, instances of value types are not necessarily immutable, only
the *value* they represent is.  For example, I can mutate a
`std::vector`_ through its ``push_back()`` method.  Or I can modify a
``struct`` by assigning to its members.  Instead, *they are value
types because each instance of the type is independent and is a
attributed meaning through the values it represent.* This **principle
of independence** is highlighted by this code snippet:

.. code-block:: c++

   auto a = std::vector<int>{};
   auto b = a;
   foo(b);
   assert(a.empty());

Here we have an instance of an empty ``std::vector`` named ``a`` and
copy it into another instance ``b``.  Then we call ``foo()`` and
finally make an assertion about the state of ``a``.  We do not need to
know anything about what ``foo()`` does to reason about the state of
``a``. It could have taken ``b`` by reference and changed it, maybe
even from another thread---it does not matter.  In any case, we can be
sure that our assertion about ``a`` holds, because ``b`` is an
independent copy of it, so we can not observe any change in ``b``
through ``a``.  This frees our mind, allowing us to reason locally
about the state of the program.  This is fundamental to write correct
concurrent programs: we can make sure each thread works on their local
copy of the data to ensure the absence of race conditions.

.. _value type: https://en.wikipedia.org/wiki/Value_type_and_reference_type
.. _std::vector: https://en.cppreference.com/w/cpp/container/vector
.. _functional programming: https://en.wikipedia.org/wiki/Functional_programming

.. admonition:: Spotting reference types

   In order to avoid them, it is important to be able to clearly
   identify *reference types*.  They are sneaky.  For example,
   ``std::vector<T>`` is a value type only as long as ``T`` is also a
   value type.  Same applies to ``struct``, it is not a value as soon
   as a member is not.

   Also, there are some C++ types that behave a bit like values, they
   seem to be copyable at least---but their copies reference external
   entities that mutate, and these changes can be observed through all
   these shallow copies!  Here are some examples of such types that
   shall be avoided in our data-model:

   * A **pointer**.  A pointer has a value, which is the memory location
     of an object. But we can also use this pointer value to change
     and observe changes in this memory location.

   * An **iterator** into a container, as mentioned above.

   * A **file handle**.  The file changes independently and we can use
     the handle to observe or make changes.  The file can even change
     outside of our program, completely outside of our control.

   * Most **resource handles** actually. Ids of OpenGL buffers, Posix
     process handles, etc. Lots of these things look like an ``int``,
     but they are not, they are a reference to a mutable entity.

   * **Function objects** that capture references, particularly lambdas
     with ``&``-captures.

   **Are pointers always bad?**

   Not always. For instance, the *implementation* of ``std::vector``
   uses pointers internally.  But it defines ``operator=`` to ensure
   the independence principle.  Also, Sean Parent shows in his famous
   talk `"Inheritance is the base class of evil"`_ that given a value
   type ``T``, a forest of ``std::shared_ptr<const T>`` is
   effectively a value type.  However, the ``immer::box`` type from
   the Immer_ library serves the same purpose with safer and more
   convenient interface.  In the :ref:`performance` section we will
   cover the power immutability in more detail.

   In general, the application level data model must never use
   pointers.  It can however use generic container types that
   encapsulate the pointers and properly deal with the intricacies of
   defining ``operator=`` as to ensure the independence principle.

.. _"Inheritance is the base class of evil": https://www.youtube.com/watch?v=bIhUE5uUFOA
.. _immer: https://github.com/arximboldi/immer

.. admonition:: Further reading
   :class: note

   Value-semantic thinking is a vast topic.  Here are some resources
   that can help you adopt this design mindset:

   * The book `Elements of Programming`_, by `Alexander Stepanov`_, author of
     the STL and great thinker on the intersection between programming
     and math.

   * The talk `Better Code: Data Structures`_ by `Sean Parent`_,
     author of the Adobe Source Libraries, famous for popularizing
     `type erasure`_ for value based runtime polymorphism.

   * The talk `The most valuable values`_, by `Juan Pedro Bolívar
     Puente`_, author of this library.

   In the functional programming realm these ideas are taken much
   further:

   * The talk `The value of values`_, and the supporting essay `Values
     and Change`_, by `Rich Hickey`_, author of the programming
     language Clojure.

   * The talk `Denotational Design: from meanings to programs`_, where
     `Conal Elliott`_, inventor of `functional reactive programming`_
     among many other other things, talks about applying mathematical
     thinking to the design of programs.

   * The book `Functional Programming in C++`_ by `Ivan Čukić`_, which
     shows how C++ not only supports value semantics, but the
     functional programming paradigm as a whole.

     .. _elements of programming: http://elementsofprogramming.com
     .. _alexander stepanov: https://en.wikipedia.org/wiki/Alexander_Stepanov
     .. _better code\: data structures: https://www.youtube.com/watch?v=sWgDk-o-6ZE
     .. _sean parent: https://sean-parent.stlab.cc/
     .. _the most valuable values: https://www.youtube.com/watch?v=_oBx_NbLghY
     .. _the value of values: https://www.youtube.com/watch?v=-6BsiVyC1kM
     .. _values and change: https://clojure.org/about/state
     .. _juan pedro bolívar puente: http://sinusoid.al
     .. _denotational design\: from meanings to programs: https://www.youtube.com/watch?v=bmKYiUOEo2A
     .. _functional programming in c++: https://www.manning.com/books/functional-programming-in-c-plus-plus
     .. _Ivan Čukić: https://cukic.co/
     .. _conal elliott: http://conal.net/
     .. _functional reactive programming: https://en.wikipedia.org/wiki/Functional_reactive_programming
     .. _type erasure: https://www.youtube.com/watch?v=QGcVXgEVMJg
     .. _rich hickey: https://twitter.com/richhickey

.. _identity:
Identity
--------

.. image:: _static/identity.png
   :align: center

When writing the model as value types, we soon encounter the problem
of dealing with **identity**. Consider an interactive application
shows a moving person. This person *changes*, it moves around.  Our
model would be a *snapshot* of the *state* of this person.  But
clearly, the *state* of the *person* is different than the person
itself:

* The **same** person can be in different states, this is, these state
  values
  are ``!=``.

* Two **different** people can be in the same state, this is, their
  state values are ``==``.

In Object Oriented programming, we normally *identify* a language
object with the entity it represents, in this case, the person.  The
*identity* of the thing is the memory location of the storage for its
state. This means that we need to use mutation to deal with the state,
that there is only one state per entity at time, that time only
progresses in one direction, that change is an implicit construction,
that entities can not be dealt with concurrently.  Identity becomes an
implicit and flaky construction.

However, in real life, we deal with identity in an explicit way.  That is
why people have *names* or *passport numbers*.  These are special
values, **identity values**, that help us identify people. Identity as
such serves a double purpose, solving the forementioned state/identity
problems:

* *Identity* allows us to recognise *different states* as belonging to
  *the same entity*. For example, when you show up at different
  institutional offices, you show your id card to show that these
  are the same person.

* *Identity* allows us to *differentiate* to a specific entity, that
  might have otherwise similar states.  In a room full of people, you
  can call someone by their full name to refer to a particular person
  univocally, distinguishing them from the group.

Considering this duality, when your program deals with changing
entities, you will have to think about the domain of entities as a
whole, and give each of those entities a different, explicit, identity
value. `Universally Unique Identifiers`_ are a powerful tool to
identify entities not only in the running programm, but also across
files and machines. Often though, context will allow us to have more
lightweight identity values.  In some cases, even its index in a
vector might suffice.

.. _universally unique identifiers: https://en.wikipedia.org/wiki/Universally_unique_identifier

.. admonition:: References in a value world

   Consider this data-structure which implements agenda of people with
   friends in a referential way:

   .. code-block:: c++

      struct person
      {
          std::string name;
          std::string phone_number;
          std::vector<std::weak_ptr<person>> friends;
      };

      struct agenda
      {
          std::unordered_set<std::shared_ptr<person>> people;
      };

   This is not a valid model to use in Lager, because ``person`` and
   ``agenda`` are reference semantic types.  Not only is the
   identification of memory objects with entities problematic from a
   conceptual programming sense: there is some extra friction, like
   having to allocate each person in a separate memory block (instead
   of having a flat ``std::vector``), and then dealing with the
   lifetime of those blocks with ``shared_ptr``, ``weak_ptr``, and so
   on.

   How do we do references with out pointers then? We use
   explicit identity values:

   .. code-block:: c++
      :emphasize-lines: 1,7,12

      using person_id = std::string;

      struct person
      {
          std::string name;
          std::string phone_number;
          std::vector<person_id> friends;
      };

      struct agenda
      {
          std::unordered_map<person_id, person> people;
      };

   Now we have decoupled the *identity* (``person_id``) from the
   *state* (``person``). Whenever we want to know the state for a
   given person, we can access it through the ``people`` map in the
   agenda.  Whenever we want to refer to a person, like in the list of
   friends, we use a ``person_id``.  We can now have multiple copies
   of the whole agenda, and compare how a particular person changes as
   we manipulate it.  People are not tied to their representation in
   memory anymore, so we can be more playful with the data-structures
   and apply :ref:`data oriented design <data-oriented-design>` to
   reach better cache locality and and overall performance!

.. _normalization:

Normalization
-------------

After applying the principle of explicit identity to your program, you
might realise this insight: *the data-model of the application starts
to look like a data-base!*

And you are correct: the model of our application is an in-memory
data-base, and the Lager store, combined with reducers and actions,
provide a reproducible, logic aware, `event sourced`_ data-store.
This means that you can start applying data-base design wisdom to your
application, which has been accrued over decades by academics and
practitioners.  In particular, you may find interesting the notion of
`database normalization`_, both the Redux documentation and the
Data-Oriented Design book do indeed talk about it:

* The `Normalizing State Shape`_ section from the Redux documentation
  discusses normalization in the context the unidirectional
  data-flow architecture.

* The `Relational Databases`_ section, from the *Data-Oriented Design*
  book by Richard Fabian, discusses normalization of the in memory
  model of C++ programs, with special focus on performance.

.. _event sourced: https://martinfowler.com/eaaDev/EventSourcing.html
.. _database normalization: https://en.wikipedia.org/wiki/Database_normalization
.. _normalizing state shape: https://redux.js.org/recipes/structuring-reducers/normalizing-state-shape
.. _relational databases: http://www.dataorienteddesign.com/dodbook/node3.html

.. _performance:
Performance
-----------

One strong concern when applying value-semantics for the data-model of
big applications is performance.  We encourage passing by value around
freely, and storing copies of the model values as needed without much
concern.  This often rises skepticism from experienced developers with
an eye for optimization.  For big data-models, isn't that gonna be
slow, and even explode the memory usage?  Not necessarily.

In C++, we normally associate value-semantics, and in particular the
:ref:`independence principle<independence-principle>`, with deep
copying.  For standard containers like ``std::vector``, it is the case
that whenever we pass by value, a new memory object is created where
the whole representation of the value is copied into.  This is however
not a consequence of value-semantics, but a consequence of mutability!
If the object that stores the value can mutate arbitrarily, when
passed by value, all of its contents must be copied to ensure that the
new object does not change when the original changes.  However, if the
original object is in some way **immutable**, the immutable parts of
the representation can be internally shared accross all the "copies"
of the value. This property is called *structural sharing* and makes
value-semantics very efficient!

In the field of `persistent data-structures`_ we can find many
examples of containers designed with the *structural sharing* in mind.
Today, we have good implementations of some of these data-structures
in C++:

* Immer_, **immutable data-structures for C++**.
* `Postmodern immutable data-structures`_, CppCon'18 talk about Immer.
* `Persistence for the masses`_, ICFP'17 paper on immutable
  data-structures in C++.

.. _immer: https://github.com/arximboldi/immer
.. _postmodern immutable data-structures: https://www.youtube.com/watch?v=sPhpelUfu8Q
.. _persistence for the masses: https://public.sinusoid.es/misc/immer/immer-icfp17.pdf
.. _persistent data-structures: https://en.wikipedia.org/wiki/Persistent_data_structure

Also, in modern C++ one may often avoid copies altogether by
leveraging `copy ellision`_ and `move semantics`_.  It is important to
familiarize oneself with these concepts.

.. _copy ellision: https://en.cppreference.com/w/cpp/language/copy_elision
.. _move semantics: https://stackoverflow.com/questions/3106110/what-is-move-semantics

In practice, when combining value-oriented design with immutable
data-structures, you will find that not only is performance not a
problem, but that **your programs are faster**!  This is due to the
fact that our data-model becomes more compact, with less pointer
chasing and better cache locality, and with flatter call stacks and no
traversal of forests of listeners, signals and slots.
