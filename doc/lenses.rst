.. _lenses:

Lenses
======

A tool borrowed from functional programming. While lenses are used
extensively in Lager for zooming on cursors, they are also useful on
their own for manipulating immutable data.

.. _making-a-lens:

Making a lens
-------------

Lenses are, conceptually, a pair of functions for focusing on a part
of a whole. You use a lens with the following interface:

.. code-block:: c++

   // pseudocode for the lens interface:
   part view(lens<whole, part>, whole); // get part of a whole
   whole set(lens<whole, part>, whole, part); // set the part of a whole
   whole over(lens<whole, part>, whole, std::function<part(part)>); // update the part of a whole
   lens<A, C> compose(lens<A, B>, lens<B, C>); // compose two lenses into another lens

Let's use this mouse as our model:

.. image:: _static/mouse.svg
   :width: 50%
   :align: center

.. code-block:: c++

   struct Mouse {
       Mouth mouth;
       pair<Eye, Eye> eyes;
       vector<Leg> legs;
       vector<Whisker> whiskers;
       Tail tail;
   };

So, how do we make a lens, say, to access the eyes of a mouse?

.. code-block:: c++

   auto eyes = [](auto f) {
       return [f](Mouse mouse) {
           return f(mouse.eyes)([&](pair<Eye, Eye> eyes) {
               mouse.eyes = eyes;
               return mouse;
           });
       };
   };

This is a `van Laarhoven lens`_, which is a bit difficult to
understand at first glance. Thankfully, we provide a way to generate
this kind of construct with a pair of functions:

.. code-block:: c++

   auto eyes = lager::lenses::getset(
       // the getter (Mouse -> Eyes)
       [](Mouse mouse) { return mouse.eyes; },
       // the setter (Mouse, Eyes -> Mouse)
       [](Mouse mouse, pair<Eye, Eye> eyes) {
           mouse.eyes = eyes;
           return mouse;
       });

Now, anyone can make their own lenses without knowing all the gritty
metaprogramming details. Of course, writing all of these by hand is
kind of a pain, so we also provide a set of lens generators for a few
common patterns:

.. code-block:: c++

   #include <lager/lenses/attr.hpp>
   auto eyes = lager::lenses::attr(&Mouse::eyes);

``attr`` will generate a lens from a pointer to member.
We will go over the rest of these generators later on.

.. admonition:: Note
   :class: note

   The main takeaway from this is that lenses are just
   *pure functions*.

.. _van Laarhoven lens: https://www.twanvl.nl/blog/haskell/cps-functional-references

.. _composition:

Composition
-----------

Lenses are a fairly "new" (2007) concept, even in functional
programming. One of the main struggles functional programmers faced
with them is composition: back when lenses were known as `Accessors`_,
lens composition was a mess to write...
Thankfully, functional programmers have since found increasingly clean
ways of doing lens composition, starting with `Twan van Laarhoven's
implementation`__, and many more to come. If you're curious about the
canonical way of doing "Optics" (a superset of lenses), I invite you
to read about `Profunctor Optics`_.

__ `van Laarhoven lens`_

So how does all of this affect us? Simple: **lens composition** with
VLLs (van Laarhoven lenses) **is function composition**!

.. code-block:: c++

   #include <lager/lenses/attr.hpp>
   auto eyes = lager::lenses::attr(&Mouse::eyes);
   auto first = lager::lenses::attr(&pair<Eye, Eye>::first);
   auto first_eye = [=](auto f){ return eyes(first(f)); };

Now, because doing function composition in C++ is unfortunately
a bit verbose, we provide syntactic sugar for function composition
through ``zug::comp``:

.. code-block:: c++

   #include <lager/lenses.hpp>
   // all of these are equivalent:
   auto first_eye = [=](auto f){ return eyes(first(f)); };
   auto first_eye = zug::comp(eyes, first);
   auto first_eye = eyes | first;

.. admonition:: Zug

   `Zug`_ is a C++ transducer implementation. It is used behind the
   scenes in Lager, but you can also use it for writing cursor
   transformations. It also has a few utilities you might find useful.
   ``zug::comp`` is one of those.
   

``zug::comp`` does two things: it is able to compose any number of
functions, and it wraps them so that you can use the pipe operator to
compose them with any other function. All the lens generators in lager
(including getset) wrap their results in a ``zug::comp``, so you can
use the *pipe operator* to *compose lenses* together.

Let's look at an example of this in action:
our mouse's mouth has four incisors!

.. code-block:: c++

   struct Mouth {
       using ToothPair = pair<Tooth, Tooth>;
       // lower pair and upper pair!
       pair<ToothPair, ToothPair> incisors;
   };

Say our mouse has a bad tooth, and we need to replace it.

.. code-block:: c++

   Mouse replace_tooth(Mouse mouse, Tooth tooth) {
       auto tooth_lens = attr(&Mouse::mouth)
           | attr(&decltype(Mouth::incisors)::first)
           | attr(&Mouth::ToothPair::first);
       return set(tooth_lens, mouse, tooth);
   }

Another thing you might notice, is that *the identity for lens
composition is the identity function!*

.. code-block:: c++

   auto add4 = [](int x) { return x + 4; };
   over([](auto f) { return f; }, 11, add4) // using our own identity function
   over(zug::indentity, 11, add4) // using zug's identity function
    
   struct Foo { int value; };
   view(zug::identity | attr(&Foo::value), Foo{42});
   view(attr(&Foo::value) | zug::identity, Foo{42});

.. _zug: https://sinusoid.es/zug/
.. _accessors: http://web.archive.org/web/20071023064034/http://luqui.org/blog/archives/2007/08/05/haskell-state-accessors-second-attempt-composability/
.. _profunctor optics: https://www.cs.ox.ac.uk/people/jeremy.gibbons/publications/poptics.pdf

.. _lens-generators:

Lens generators
---------------

Let's look at the different lens generators that are available to us.
Assume the following is available:

.. code-block:: c++

   #include <lager/lenses.hpp>
   using namespace lager;
   using namespace lager::lenses;
   
   Mouse mouse; // our instance of a mouse

We've already seen ``attr``:

.. code-block:: c++

   #include <lager/lenses/attr.hpp>
   auto first_eye = attr(&Mouse::eyes)
           | attr(&pair<Eye, Eye>::first);
   
   Eye eye = view(first_eye, mouse);

``at`` is an accessor for an element of a collection at an index
(integers for sequences like ``vector``, keys for associative
collections like ``map``):

.. code-block:: c++

   #include <lager/lenses/at.hpp>
   auto first_whisker = attr(&Mouse::whiskers) | at(0);
   
   optional<Whisker> maybe_whisker = view(first_whisker, mouse);

Note that the focus (``part``) of at is an optional. That's because
*the focused element might be absent* (out of bounds, no value at key,
etc). We'll go over handling optionals later. If you don't want to
handle optionals and you're ok with using default constructed values
as a representation of the absence of focus, you can use ``at_or``:

.. code-block:: c++

   #include <lager/lenses/at_or.hpp>
   
   // default constructing a value if none is present:
   auto with_default = attr(&Mouse::whiskers) | at_or(0);
   
   // using a fallback value:
   Whisker fallback_whisker;
   auto with_fallback = attr(&Mouse::whiskers)
           | at_or(0, fallback_whisker);
   
   auto first_whisker = with_default;
   Whisker whisker = view(first_whisker, mouse);

This is *usually* not recommended, please use ``at`` and handle
optionals properly.

Then there's handling variants:

.. code-block:: c++

   #include <lager/lenses/variant.hpp>
    
   variant<Mouse, Rat> rodent;
   auto the_mouse = alternative<Mouse>;
    
   optional<Mouse> maybe_mouse = view(the_mouse, rodent);

Similarly to ``at``, ``alternative``'s focus is an optional.

Finally because `recursive types should be implemented with boxes
<https://sinusoid.es/immer/containers.html#box>`_, we provide unbox:

.. code-block:: c++

   #include <lager/lenses/unbox.hpp>
    
   // a tail node has a position and maybe another tail node
   struct Tail {
       int position;
       box<optional<Tail>> tail;
   };
    
   auto tail = attr(&Mouse::tail)
           | attr(&Tail::tail)
           | unbox;
    
   optional<Tail> maybe_tail = view(tail, mouse);

Note that tail really should be of type ``optional<box<Tail>>``, but
for that we'd need to handle composing with optionals.

.. _handling-optionals:

Handling optionals
------------------

So many optionals everywhere! How do we compose lenses that focus on
optionals?

This is the part that gets slightly tricky: you can't compose a lens
that focuses on an optional with a lens that expects a value. But you
can *turn a lens that expects a value into a lens that expects an
optional!*

We provide three ways of doing this. Assume the following is
available:

.. code-block:: c++

   #include <lager/lenses.hpp>
   #include <lager/lenses/optional.hpp>
   #include <lager/lenses/at.hpp>
   #include <lager/lenses/attr.hpp>
   using namespace lager;
   using namespace lager::lenses;
    
   struct Mouse; // from earlier
   struct Digit { int position; };
   struct Leg {
       int position;
       vector<Digit> digits;
   };
    
   Mouse mouse; // our instance of a mouse

The first one is ``map_opt``:

.. code-block:: c++

   auto leg_position = attr(&Leg::position);
   auto first = at(0);
   auto first_leg_position = attr(&Mouse::legs) // vector<Leg>
           | first                              // optional<Leg>
           | map_opt(leg_position);             // optional<int>
    
   optional<int> position = view(first_leg_position, mouse);

``map_opt`` turned our ``lens<Leg, int>`` into a
``lens<optional<Leg>, optional<int>>``. This is one way to lift
lenses to handle optionals.

Now, what happens if we try to do the same thing to get the first
``Digit`` of the first ``Leg``?

.. code-block:: c++

   auto digits = attr(&Leg::digits);
   auto first = at(0);
   auto first_digit = attr(&Mouse::legs) // vector<Leg>
           | first                       // optional<Leg>
           | map_opt(digits)             // optional<vector<Digit>>
           | map_opt(first);             // optional<optional<Digit>>
    
   optional<optional<Digit>> digit = view(first_digit, mouse);

Oh no. We got an optional of optional, which is not what we wanted.
We wanted to turn our ``lens<vector<Digit>, optional<Digit>>`` into a
``lens<optional<vector<Digit>>, optional<Digit>>``.

For this, we have ``bind_opt``:

.. code-block:: c++

   auto first_digit = attr(&Mouse::legs) // vector<Leg>
           | first                       // optional<Leg>
           | map_opt(digits)             // optional<vector<Digit>>
           | bind_opt(first);            // optional<Digit>
    
   optional<Digit> digit = view(first_digit, mouse);

Note that you can lift composed lenses too!

.. code-block:: c++

   auto first_digit = attr(&Mouse::legs) // vector<Leg>
           | first                       // optional<Leg>
           | bind_opt(digits | first);   // optional<Digit>

``bind_opt`` collapses two levels of optional into one, much like the
monadic bind of the `Maybe Monad`_ (don't think too much about it).

For convenience, we also provide ``with_opt``, which will
automatically attempt to collapse two levels of optionals if it finds
any:

.. code-block:: c++

   auto first_digit = attr(&Mouse::legs) // vector<Leg>
           | first                       // optional<Leg>
           | with_opt(digits | first);   // optional<Digit>
   optional<Digit> digit = view(first_digit, mouse);
    
   auto first_leg_position = attr(&Mouse::legs) // vector<Leg>
           | first                              // optional<Leg>
           | with_opt(leg_position);            // optional<int>
   optional<int> position = view(first_leg_position, mouse);

This should be safe to use, but be weary of using it with models that
have optionals as legitimate values. Using the less ambiguous
``map_opt`` and ``bind_opt`` is preffered.

Of course, we also provide a lens for falling back to either a
default constructed value or a fallback value with ``value_or`` and
``or_default``:

.. code-block:: c++

   auto first_leg_position = attr(&Mouse::legs) // vector<Leg>
           | first                              // optional<Leg>
           | map_opt(leg_position);             // optional<int>
    
   auto with_default = first_leg_position | or_default; // default constructed
   // auto with_default = first_leg_position | value_or(); // equivalent
   auto with_fallback = first_leg_position | value_or(-1); // fallback to -1
    
   int position = view(with_fallback, mouse);


.. _maybe monad: https://en.wikipedia.org/wiki/Monad_(functional_programming)

.. _dynamic-lenses:

Dynamic lenses
--------------

You've probably noticed that all of our lenses have the type ``auto``
in the previous examples. This is because VLLs rely on *compile-time
type information* to implement ``view``, ``set`` and ``over``, and the
resulting types are somewhat cryptic... This is fine for composing
lenses at compile time, but here's the catch:

.. code-block:: c++

   struct Tail {
       int position;
       optional<box<Tail>> tail;
   };
    
   auto tail = attr(&Tail::tail) | value_or() | unbox;
   auto position = attr(&Tail::position);
    
   auto lens1 = tail | position;        // lens<Tail, int>
   auto lens2 = tail | tail | position; // lens<Tail, int>
    
   static_assert(std::is_same_v<decltype(lens1), decltype(lens2)>,
                 "Not the same types!");

This means that you can't have this kind of pattern:

.. code-block:: c++

   auto tail_position_at(int index) {
       auto result_lens = position;
       while(index-- > 0) {
           result_lens = tail | result_lens; // won't compile, the type changed!
       }
       return result_lens;
   }

We need a way to store ``lens1`` and ``lens2`` in the same type,
because they satisfy the same interface that we defined in
`Making a lens`_ (they are both, conceptually, ``lens<Tail, int>``).

This is where *type erasure* comes in:

.. code-block:: c++

   #include <lager/lens.hpp> // type erased lenses
    
   lens<Tail, int> tail_position_at(int index) {
       lens<Tail, int> result_lens = position;
       while (index-- > 0) {
           result_lens = tail | result_lens; // this works now
       }
       return result_lens;
   }

The ``<lager/lens.hpp>`` header provides a type erased lens for this
very purpose. This is achieved through the same technique used for
implementing ``std::function``.

.. admonition:: Virtual dispatch overhead
   :class: warning

   Type erased lenses are less performant at runtime, because of
   virtual dispatch, and because we can't take advantage of a number
   of optimizations done by VLLs. For this reason, **do not use type
   erased lenses if you can express something equivalent at compile
   time**. (``std::function`` suffers from similar limitations, and as
   such follows the same recommendations)

Let's reimplement that last function one last time, with proper
handling of optionals this time:

.. code-block:: c++

   auto tail = attr(&Tail::tail) | map_opt(unbox);
   auto position = attr(&Tail::position) | force_opt;
    
   lens<Tail, optional<int>> tail_position_at(int index) {
       lens<Tail, optional<int>> result_lens = position;
       while (index-- > 0) {
           result_lens = tail | bind_opt(result_lens);
       }
       return result_lens;
   }

Notice that we introduced ``force_opt``. This is so that we can keep
the return type as ``lens<Tail, optional<int>>``, even in the case of
a single node tail.

