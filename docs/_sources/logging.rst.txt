Configuring Logging for NMSLIB
==============================

This library logs to a Python logger named ``nmslib``. This lets
you fully control the log messages produced by nmslib in Python.

For instance, to log everything produced by nmslib to a default
python logger:

.. code-block:: python

    # setup basic python logging
    import logging
    logging.basicConfig(level=logging.DEBUG)

    # importing nmslib logs some debug messages on startup, that
    # that will be output to the python log handler created above
    import nmslib

To quiet these messages you can just set the level for nmslib
as appropiate:

.. code-block:: python

    # setup basic python logging
    import logging
    logging.basicConfig(level=logging.DEBUG)

    # Only log WARNING messages and above from nmslib
    logging.getLogger('nmslib').setLevel(logging.WARNING)

    import nmslib
