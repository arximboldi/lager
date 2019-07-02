
.. _modularity:

Modularity
==========

Until now we have learnt how to build simple self-contained
components.  However, in real world systems, you will have multiple
module that interact with eachother in various ways, and are often
developed by different people and teams.  In this section, we will
learn how to scale up our Lager based application.

Composability
-------------

We use the term **module** to refer to a set of :ref:`actions`,
:ref:`model`, :ref:`reducers`, and optionally :ref:`effects` and
:ref:`views`.  This will be the unit of composition in our system.

As an example, we will describe a system with two modules, ``foo`` and
``bar``, that are composed into a bigger module, ``app``.  Unlike in
an object-oriented system, where relationships are often hidden behind
callbacks and associations, we will use simple explicit composition:
data members and function calls.

.. image:: _static/composition.svg
   :width: 80%
   :align: center

.. admonition:: Horizontal vs vertical physical organization

   It might be tempting to organize your program in a *horizontal* or
   *layered* manner.  This is, to have separate folders foll all your
   actions, models, reducers and views.  If your model is reified in
   different UI's, maybe belonging to different applications, it might
   actually make sense to keep the views separate.  However, actions,
   models and reducers are intimatelly tied to eachother, representing
   different aspects of the same interface.  For this reason, it makes
   sense to keep their definitions close, in the same folder or maybe
   even in the same file.  This is what we call *vertical*
   modularization.

   In this way, your code is organized not around arbitrary technical
   definitions, but around the features and aspects of your
   application.  As you scale up your development organization, this
   will make it easier to work on various features as autonomous
   cross-functional teams that integrate product management, design,
   and full stack development. The unidirectional data-flow design
   proposed by Lager helps building clear interfaces between these
   modules that reduce friction at the component, cross-team boundary.

   .. image:: _static/modules.svg
      :width: 100%
      :align: center

Composing models
~~~~~~~~~~~~~~~~



Composing reducers
~~~~~~~~~~~~~~~~~~

Composing effects
~~~~~~~~~~~~~~~~~

Composing views
~~~~~~~~~~~~~~~



.. _undo:
.. _genericity:

Genericity
----------

Actors
------
