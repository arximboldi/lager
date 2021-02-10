.. image:: https://github.com/arximboldi/lager/workflows/test/badge.svg
   :target: https://github.com/arximboldi/lager/actions?query=workflow%3Atest+branch%3Amaster
   :alt: Github Actions Badge

.. image:: https://codecov.io/gh/arximboldi/lager/branch/master/graph/badge.svg
   :target: https://codecov.io/gh/arximboldi/lager
   :alt: CodeCov Badge

.. image:: https://cdn.jsdelivr.net/gh/arximboldi/lager/doc/_static/sinusoidal-badge.svg
   :target: https://sinusoid.al
   :alt: Sinusoidal Engineering badge
   :align: right

.. raw:: html

  <img width="100%"
       src="https://cdn.rawgit.com/arximboldi/lager/ccb5a1c8/resources/logo-front.svg"
       alt="Logotype">

.. include:introduction/start

**lager** is a C++ library to assist `value-oriented design`_ by
implementing the `unidirectional data-flow architecture`_.  It is
heavily inspired by Elm_ and Redux_, and enables composable designs by
promoting the use of simple value types and testable application logic
via pure functions.  And you get time-travel for free!

.. _unidirectional data-flow architecture: https://www.exclamationlabs.com/blog/the-case-for-unidirectional-data-flow
.. _Elm: https://guide.elm-lang.org/architecture
.. _Redux: https://redux.js.org/introduction/getting-started
.. _value-oriented design: https://www.youtube.com/watch?v=_oBx_NbLghY

* **Documentation** (Contents_)
* **Code** (GitHub_)
* **CppRussia-Piter 2019 Talk**: *Squaring the circle* (`YouTube
  <https://www.youtube.com/watch?v=e2-FRFEx8CA>`_, `Slides
  <https://sinusoid.es/talks/cpprussia19-piter>`_)
* **CppCon 2018 Talk**: *The most valuable values* (`YouTube
  <https://www.youtube.com/watch?v=_oBx_NbLghY>`_, `Slides
  <https://sinusoid.es/talks/cppcon18>`_)
* **C++ on Sea 2019 Talk**: *Postmodern immutable data-structures*
  (`YouTube <https://www.youtube.com/watch?v=y_m0ce1rzRI>`_, `Slides
  <https://sinusoid.es/talks/cpponsea19>`_)

.. _contents: https://sinusoid.es/lager/#contents
.. _github: https://github.com/arximboldi/lager

  .. raw:: html

     <a href="https://www.patreon.com/sinusoidal">
         <img align="right" src="https://cdn.rawgit.com/arximboldi/immer/master/doc/_static/patreon.svg">
     </a>

  This project is part of a long-term vision helping interactive and
  concurrent C++ programs become easier to write. **Help this project's
  long term sustainability by becoming a patron or buying a
  sponsorship package:** juanpe@sinusoid.al

.. include:index/end

Examples
--------

For a guided introductory tour with **code samples**, please read the
`architecture overview`_ section. Other examples:

.. _architecture overview: https://sinusoid.es/lager/architecture.html

* **Counter**, a minimalistic example with multiple UIs (`link
  <https://github.com/arximboldi/lager/tree/master/example/counter>`_).
* **Autopong**, a basic game using SDL2 (`link
  <https://github.com/arximboldi/lager/blob/master/example/autopong>`_).
* **Ewig**, a terminal text editor with undo, asynchronous loading,
  and more (`link <https://github.com/arximboldi/ewig>`_).

Why?
----

Most interactive software of the last few decades has been written
using an object-oriented interpretation of the `Model View
Controller`_ design.  This architecture provides nice separation of
concerns, allowing the core application logic to be separate from the
UI, and a good sense of modularity.  However, its reliance on stateful
object graphs makes the software hard to test or parallelize.  It's
reliance on fine-grained callbacks makes composition hard, resulting
in subtle problems that are hard to debug.

*Value-based unidirectional data-flow* tackles a few of these
problems:

* Thanks to immutability_ and value-types, it is very easy to add
  **concurrency** as threads can operate on their local copies of the
  data without mutexes or other flaky synchronization mechanisms.
  Instead, worker threads communicate their results back by *dispatching*
  actions to the main thread.

* The application logic is made of `pure functions`_ that can be easily
  **tested** and are fully reproducible.  They interact with the world
  via special side-effects procedures loosely coupled to the services
  they need via  `dependency injection`_.

* This also means that data and call-graphs are always trees or
  `DAGs`_ (instead of cyclical graphs), with *explicit composition*
  that is to trace and **debug**.  You can also always
  *snapshot* the state, making undo and time-travel easy peasy!

.. _immutability: https://github.com/arximboldi/immer
.. _pure functions: https://en.wikipedia.org/wiki/Pure_function
.. _model view controller:
   https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93controller
.. _dependency injection:
   https://en.wikipedia.org/wiki/Dependency_injection
.. _DAGs: https://en.wikipedia.org/wiki/Directed_acyclic_graph

Dependencies
------------

This library is written in **C++17** and a compliant compiler and
standard library necessary.  It is `continuously tested`_ with GCC 7,
but it might work with other compilers and versions.

It also depends on `Zug`_ and `Boost Hana`_. Some optional extensions and modules
may have other dependencies documented in their respective sections.

.. _Zug: https://github.com/arximboldi/zug/
.. _Boost Hana: https://boostorg.github.io/hana
.. _continuously tested: https://travis-ci.org/arximboldi/immer

Usage
-----

This is a **header only** you can just copy the ``lager`` subfolder
somewhere in your *include path*.

Some components, like the time-travelling debugger, also require the
installation of extra files.

You can use `CMake`_ to install the library in your system once you
have manually cloned the repository::

    mkdir -p build && cd build
    cmake .. && sudo make install

.. _nix package manager: https://nixos.org/nix
.. _cmake: https://cmake.org/

Development
-----------

In order to develop the library, you will need to compile and run the
examples, tests and benchmarks.  These require some additional tools.
The easiest way to install them is by using the `Nix package
manager`_.  At the root of the repository just type::

    nix-shell

This will download all required dependencies and create an isolated
environment in which you can use these dependencies, without polluting
your system.

Then you can proceed to generate a development project using `CMake`_::

    mkdir build && cd build
    cmake ..

From then on, one may build and run all tests by doing::

    make check

License
-------

.. image:: https://raw.githubusercontent.com/arximboldi/lager/docs/doc/_static/mit.png
   :alt: Boost logo
   :target: https://opensource.org/licenses/MIT
   :align: right
   :width: 140 px

**This software is licensed under the MIT license**.

The full text of the license is can be accessed `via this link
<https://opensource.org/licenses/MIT>`_ and is also included in the
``LICENSE`` file of this software package.
