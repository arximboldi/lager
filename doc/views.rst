
.. _views:

Views
=====

After the data-model is updated via a :ref:`reducer`, the ``view``
function provided to the store is invoked to update the UI.  There are
various ways to implement a view in the Lager model.

Value based UI
--------------

In an ideal world, the *view* is just :ref:`a value<value-semantics>`
and the user interface is a function that takes a *model* and returns
such *view*.  This fits perfectly the unidirectional data-flow architecture.

However, to do make this pattern convenient and efficient, you need a
framework.  In the JavaScript world, `virtual DOM`_ frameworks like
React_ allow you to conveniently represent the UI as a value, and
using this declarative description to efficiently manipulate an
underlying :ref:`UI object tree<object-ui>`.  Sadly, in the C++ world,
no such library exists. This approach is therefore not very practical
for Lager applications at the moment.

.. admonition:: Support the development of a value-based UI library for C++!
   :class: danger

   Thanks to value semantics and static polymorphism, C++ would indeed
   be a great language for a React-like library.  We at `Sinusoidal
   Engineering`_ spend a lot of time thinking about how to further
   value-oriented design in C++, and have a bunch of ideas on how to
   build a declarative UI framework that does not reinvent the wheel
   and can be plugged on top of existing native frameworks like
   `Gtk+`_ or Qt_, Cocoa or the Windows API. This is however a non
   trivial project. Help us save the planet: no more inefficient
   Electron apps!

   `Contact us to have your company sponsor such project!`_

.. _virtual dom: https://reactjs.org/docs/faq-internals.html#what-is-the-virtual-dom
.. _react: https://reactjs.org/
.. _sinusoidal engineering: http://sinusoid.al/
.. _qt: https://www.qt.io/
.. _gtk+: https://www.gtk.org
.. _Contact us to have your company sponsor such project!: mailto:juanpe@sinusoid.al

Immediate mode UI
-----------------

A practical alternative is the usage of `immediate mode UI`_. There
are a few UI libraries following this approach and they are specially
popular for the development of video games.  Popular ones are ImGUI_
and FlatUI_.

In an *immediate mode UI*, whenever you need to redraw, you traverse
your whole state tree, and invoke primitives of the framework as you
go to represent it, drawing buttons, input boxes, labels, windows,
etc.  The architecture of a Lager application fits this model very
well: our state is composed of simple value types that are easy to
traverse.

Furthermore, using Lager helps solve some of the problems that
immediate mode UI's have when scaling.  One is *state tearing*. In a
naive immediate mode UI, you the state as you draw it.  This means
that if the same piece of state is represented in multiple places, it
might be inconsistent in a frame, as the mutation happens during the
rendering.  When using Lager with an immediate mode UI framework, you
can not mutate the state directly, but
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
tree represented by mutable objects.

These are designed around the idea that your model is also a mutable
object tree, that you can observe via listeners.  Whenever the model
is changed, a listener manipulates the widget tree to keep it updated.
Whenever the user manipulates a widget, another listener updates the
model.  This is the kind of circular relationship that we wanted to
avoid with the unidirectional data-flow.

You can still use an unidirectional data-flow approach in combination
with a widget tree UI, by breaking the circle as follows:

- You  listeners to the widgets as usual to observe user
  interactions, but you can not mutate the model.  Instead, you
  dispatch actions.

- The model is composed of value types with no observers, so there are
  no listeners to connect there.  But you can easily copy it.  So you
  keep a copy of the last version around, such that when the new state
  of the whole application is pushed, you traverse the state,
  comparing the old and new value.  When you find discrepancies, you
  update the widget tree in the same way you would do in a listener.

.. admonition:: Library support

   This diffing mechanism can be a bit cumbersome, and sometimes error
   prone. At `Sinusoidal Engineering`_ we are developing a new library
   to aid this particular usecase.  It will be first presented at `C++
   Russia Piter`_ and `Meeting C++`_ in Autumn 2019.

.. _C++ Russia Piter: https://cppconf-piter.ru/en/
.. _Meeting C++: http://meetingcpp.com/

Observables
-----------

One way to look at the state of a Lager application, is as a sequence
of values over time.  Using this understanding, we can apply the
`reactive programming`_ paradigm to manipulate it.

The `RxCpp`_ library is precisely design to work with sequences of
values that change over time, and manipulate this sequence using
higher order transformations and scheduling combinators.  As such, you
can use the ``view`` function you pass to the store to push these into
an Rx observable that feeds into other reactive subsystems.

.. _reactive programming: http://reactivex.io/intro.html
.. _RxCpp: https://github.com/ReactiveX/RxCpp
