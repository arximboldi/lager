
.. _views:

Views
=====

After the data-model is updated via a :ref:`Reducer<reducers>`, all
views connected with the :cpp:func:`lager::watch` function are called.
There are various ways to implement a view in the Lager model.

Value based UI
--------------

In an ideal world, the *view* is just :ref:`a value<value-semantics>`
and the user interface is a function that takes a *model* and returns
such *view*.  This fits perfectly the unidirectional data-flow architecture.

However, to do make this pattern convenient and efficient you need a
supporting framework.  In the JavaScript world, `virtual DOM`_
frameworks like React_ allow us to conveniently represent the UI as a
value, and use this declarative description to efficiently manipulate
an underlying :ref:`UI object tree<object-ui>`.  Sadly, in the C++
world, no such library exists.

This approach is therefore not practical for Lager applications at the
moment.

.. admonition:: Support the development of a value-based UI library for C++!
   :class: danger

   Thanks to value semantics and static polymorphism, C++ would indeed
   be a great language for a React-like library.  We at `Sinusoidal
   Engineering`_ spend a lot of time thinking about how to further
   value-oriented design in C++, and have quite a bunch of ideas on
   how to build a declarative UI framework, one that does not reinvent
   the wheel and can be plugged on top of existing native frameworks
   like `Gtk+`_ or Qt_, Cocoa or the Windows API.

   This is however a non trivial project. *Help us save the planet: no
   more inefficient Electron apps!* Please consider sponsoring that
   project so that we can invest the time required to develop it, and
   also tailor it to your particular needs. `Contact us to make it
   happen!`_

.. _virtual dom: https://reactjs.org/docs/faq-internals.html#what-is-the-virtual-dom
.. _react: https://reactjs.org/
.. _sinusoidal engineering: http://sinusoid.al/
.. _qt: https://www.qt.io/
.. _gtk+: https://www.gtk.org
.. _Contact us to make it happen!: mailto:juanpe@sinusoid.al

Immediate mode UI
-----------------

A practical alternative is the usage of `immediate mode UI`_. There
are a few UI libraries following this approach and they are specially
popular for the development of video games.  These include ImGUI_ and
FlatUI_.

In an *immediate mode UI*, whenever you need to redraw, you traverse
your whole state tree invoking functions of the framework as you go to
represent it, drawing buttons, input boxes, labels, windows, etc.  The
architecture of a Lager application fits this model very well: our
state is composed of simple value types that are easy and fast to
traverse.

Furthermore, using Lager helps solve some of the problems that
immediate mode UI's have when scaling.  One is *state tearing*. In a
naive immediate mode UI, the state is mutated as you draw it, often
directly by the framework primitives.  This means that if the same
piece of state is represented in multiple places, it might be
inconsistent in a frame, as the mutation happens during the rendering.
When using Lager with an immediate mode UI framework, you can not
mutate the state directly, but
:cpp:func:`dispatch<lager::store::dispatch>` :ref:`actions<actions>`
to trigger a state change cascade.  Lager works in a transactional
way, making all mutations traceable to the actions that triggered
them, and ensuring that always consistent snapshots are rendered.

.. admonition:: Exercise
   :class: tip

   Write an ImGUI interface for the todo-list app core
   core :ref:`we described before<todo-actions>`.  If you want it to
   be reviewed and potentially included as an offical Lager example,
   `make a pull request`_ to the project.

.. _imgui: https://github.com/ocornut/imgui
.. _flatui: http://google.github.io/flatui
.. _immediate mode UI: https://en.wikipedia.org/wiki/Immediate_mode_(computer_graphics)
.. _make a pull request: https://github.com/arximboldi/lager/pulls

.. _object-ui:

Widget tree UI
--------------

Most C++ UI frameworks, like `Gtk+`_ or Qt_, are based on a widget
tree represented by mutable objects, that notify events through
listeners.

These are designed around the assumption that your model is also a
mutable object tree, that you can observe via listeners.  Whenever the
model is changed, a listener manipulates the widget tree to keep it
updated.  Whenever the user manipulates a widget, another listener
manipulates the model.  This is the kind of circular relationship that
we wanted to avoid with the unidirectional data-flow.

We can still use an unidirectional data-flow approach in combination
with a widget tree UI, by breaking the circle as follows:

- We connect listeners to the widgets as usual to observe user
  interactions, but they do not mutate the model directly.  Instead,
  they *dispatch* actions.

- There are no listeners connected to the model, that is indeed
  impossible, because the model is a value type.  But the model can be
  easily copied, so we can *diff* it.  We keep a copy of the last
  version around, such that when the new state of the application is
  pushed, we traverse the state, comparing the old and new values.
  When discrepancies are found, we update the widget tree in the same
  way we would do inside a listener.

.. admonition:: Library support

   This diffing mechanism can be a bit cumbersome, and sometimes error
   prone. :ref:`cursors` can do it for you automatically.  They can
   also do much more, and are an invaluable tool when `interfacing a
   value-oriented data model with an object-oriented
   UI<https://www.youtube.com/watch?v=e2-FRFEx8CA>`.

Observables
-----------

Another way to look at the state of a Lager application is as a
*sequence of values over time*.  Leveraging this realisation, we can
apply the `reactive programming`_ paradigm to manipulate it.

The `RxCpp`_ library is precisely designed to work with sequences of
values that change over time.  These sequences can be reified as
values called *observables* that can be manipulated using higher order
transformations and scheduling combinators.  We can use the ``view``
function that is passed to the store to push these into an Rx
observable.  This is then used to feed other subsystems in a reactive
manner.  We can also use Rx observables to source the actions into the
store.

.. _reactive programming: http://reactivex.io/intro.html
.. _RxCpp: https://github.com/ReactiveX/RxCpp
