.. index::
    single: DependencyInjection; Service Subscribers

Service Subscribers & Locators
==============================

.. versionadded:: 3.3

    Service subscribers and locators were introduced in Symfony 3.3.

Sometimes, a service needs access to several other services without being sure
that all of them will actually be used. In those cases, you may want the
instantiation of the services to be lazy. However, that's not possible using
the explicit dependency injection since services are not all meant to
be ``lazy`` (see :doc:`/service_container/lazy_services`).

This can typically be the case in your controllers, where you may inject several
services in the constructor, but the action executed only uses some of them.
Another example are applications that implement the `Command pattern`_
using a CommandBus to map command handlers by Command class names and use them
to handle their respective command when it is asked for::

    // src/AppBundle/CommandBus.php
    namespace AppBundle;

    // ...
    class CommandBus
    {
        /**
         * @var CommandHandler[]
         */
        private $handlerMap;

        public function __construct(array $handlerMap)
        {
            $this->handlerMap = $handlerMap;
        }

        public function handle(Command $command)
        {
            $commandClass = get_class($command);

            if (!isset($this->handlerMap[$commandClass])) {
                return;
            }

            return $this->handlerMap[$commandClass]->handle($command);
        }
    }

    // ...
    $commandBus->handle(new FooCommand());

Considering that only one command is handled at a time, instantiating all the
other command handlers is unnecessary. A possible solution to lazy-load the
handlers could be to inject the main dependency injection container.

However, injecting the entire container is discouraged because it gives too
broad access to existing services and it hides the actual dependencies of the
services. Doing so also requires services to be made public, which isn't the
case by default in Symfony applications.

**Service Subscribers** are intended to solve this problem by giving access to a
set of predefined services while instantiating them only when actually needed
through a **Service Locator**, a separate lazy-loaded container.

Defining a Service Subscriber
-----------------------------

First, turn ``CommandBus`` into an implementation of :class:`Symfony\\Component\\DependencyInjection\\ServiceSubscriberInterface`.
Use its ``getSubscribedServices()`` method to include as many services as needed
in the service subscriber and change the type hint of the container to
a PSR-11 ``ContainerInterface``::

    // src/AppBundle/CommandBus.php
    namespace AppBundle;

    use AppBundle\CommandHandler\BarHandler;
    use AppBundle\CommandHandler\FooHandler;
    use Psr\Container\ContainerInterface;
    use Symfony\Component\DependencyInjection\ServiceSubscriberInterface;

    class CommandBus implements ServiceSubscriberInterface
    {
        private $locator;

        public function __construct(ContainerInterface $locator)
        {
            $this->locator = $locator;
        }

        public static function getSubscribedServices()
        {
            return [
                'AppBundle\FooCommand' => FooHandler::class,
                'AppBundle\BarCommand' => BarHandler::class,
            ];
        }

        public function handle(Command $command)
        {
            $commandClass = get_class($command);

            if ($this->locator->has($commandClass)) {
                $handler = $this->locator->get($commandClass);

                return $handler->handle($command);
            }
        }
    }

.. tip::

    If the container does *not* contain the subscribed services, double-check
    that you have :ref:`autoconfigure <services-autoconfigure>` enabled. You
    can also manually add the ``container.service_subscriber`` tag.

The injected service is an instance of :class:`Symfony\\Component\\DependencyInjection\\ServiceLocator`
which implements the PSR-11 ``ContainerInterface``, but it is also a callable::

    // ...
    $handler = ($this->locator)($commandClass);

    return $handler->handle($command);

Including Services
------------------

In order to add a new dependency to the service subscriber, use the
``getSubscribedServices()`` method to add service types to include in the
service locator::

    use Psr\Log\LoggerInterface;

    public static function getSubscribedServices()
    {
        return [
            // ...
            LoggerInterface::class,
        ];
    }

Service types can also be keyed by a service name for internal use::

    use Psr\Log\LoggerInterface;

    public static function getSubscribedServices()
    {
        return [
            // ...
            'logger' => LoggerInterface::class,
        ];
    }

When extending a class that also implements ``ServiceSubscriberInterface``,
it's your responsibility to call the parent when overriding the method. This
typically happens when extending ``AbstractController``::

    use Psr\Log\LoggerInterface;
    use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;

    class MyController extends AbstractController
    {
        public static function getSubscribedServices()
        {
            return array_merge(parent::getSubscribedServices(), [
                // ...
                'logger' => LoggerInterface::class,
            ]);
        }
    }

Optional Services
~~~~~~~~~~~~~~~~~

For optional dependencies, prepend the service type with a ``?`` to prevent
errors if there's no matching service found in the service container::

    use Psr\Log\LoggerInterface;

    public static function getSubscribedServices()
    {
        return [
            // ...
            '?'.LoggerInterface::class,
        ];
    }

.. note::

    Make sure an optional service exists by calling ``has()`` on the service
    locator before calling the service itself.

Aliased Services
~~~~~~~~~~~~~~~~

By default, autowiring is used to match a service type to a service from the
service container. If you don't use autowiring or need to add a non-traditional
service as a dependency, use the ``container.service_subscriber`` tag to map a
service type to a service.

.. configuration-block::

    .. code-block:: yaml

        # app/config/services.yml
        services:
            AppBundle\CommandBus:
                tags:
                    - { name: 'container.service_subscriber', key: 'logger', id: 'monolog.logger.event' }

    .. code-block:: xml

        <!-- app/config/services.xml -->
        <?xml version="1.0" encoding="UTF-8" ?>
        <container xmlns="http://symfony.com/schema/dic/services"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="http://symfony.com/schema/dic/services https://symfony.com/schema/dic/services/services-1.0.xsd">

            <services>

                <service id="AppBundle\CommandBus">
                    <tag name="container.service_subscriber" key="logger" id="monolog.logger.event"/>
                </service>

            </services>
        </container>

    .. code-block:: php

        // app/config/services.php
        use AppBundle\CommandBus;

        // ...

        $container
            ->register(CommandBus::class)
            ->addTag('container.service_subscriber', ['key' => 'logger', 'id' => 'monolog.logger.event'])
        ;

.. tip::

    The ``key`` attribute can be omitted if the service name internally is the
    same as in the service container.

Defining a Service Locator
--------------------------

To manually define a service locator, create a new service definition and add
the ``container.service_locator`` tag to it. Use its ``arguments`` option to
include as many services as needed in it.

.. configuration-block::

    .. code-block:: yaml

        // app/config/services.yml
        services:
            app.command_handler_locator:
                class: Symfony\Component\DependencyInjection\ServiceLocator
                tags: ['container.service_locator']
                arguments:
                    -
                        AppBundle\FooCommand: '@app.command_handler.foo'
                        AppBundle\BarCommand: '@app.command_handler.bar'

    .. code-block:: xml

        <!-- app/config/services.xml -->
        <?xml version="1.0" encoding="UTF-8" ?>
        <container xmlns="http://symfony.com/schema/dic/services"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="http://symfony.com/schema/dic/services https://symfony.com/schema/dic/services/services-1.0.xsd">

            <services>

                <service id="app.command_handler_locator" class="Symfony\Component\DependencyInjection\ServiceLocator">
                    <argument type="collection">
                        <argument key="AppBundle\FooCommand" type="service" id="app.command_handler.foo"/>
                        <argument key="AppBundle\BarCommand" type="service" id="app.command_handler.bar"/>
                    </argument>
                    <tag name="container.service_locator"/>
                </service>

            </services>
        </container>

    .. code-block:: php

        // app/config/services.php
        use Symfony\Component\DependencyInjection\ServiceLocator;
        use Symfony\Component\DependencyInjection\Reference;

        // ...

        $container
            ->register('app.command_handler_locator', ServiceLocator::class)
            ->addTag('container.service_locator')
            ->setArguments([[
                'AppBundle\FooCommand' => new Reference('app.command_handler.foo'),
                'AppBundle\BarCommand' => new Reference('app.command_handler.bar'),
            ]])
        ;

.. note::

    The services defined in the service locator argument must include keys,
    which later become their unique identifiers inside the locator.

Now you can use the service locator by injecting it in any other service:

.. configuration-block::

    .. code-block:: yaml

        // app/config/services.yml
        services:
            AppBundle\CommandBus:
                arguments: ['@app.command_handler_locator']

    .. code-block:: xml

        <!-- app/config/services.xml -->
        <?xml version="1.0" encoding="UTF-8" ?>
        <container xmlns="http://symfony.com/schema/dic/services"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:schemaLocation="http://symfony.com/schema/dic/services https://symfony.com/schema/dic/services/services-1.0.xsd">

            <services>

                <service id="AppBundle\CommandBus">
                    <argument type="service" id="app.command_handler_locator"/>
                </service>

            </services>
        </container>

    .. code-block:: php

        // app/config/services.php
        use AppBundle\CommandBus;
        use Symfony\Component\DependencyInjection\Reference;

        $container
            ->register(CommandBus::class)
            ->setArguments([new Reference('app.command_handler_locator')])
        ;

In :doc:`compiler passes </service_container/compiler_passes>` it's recommended
to use the :method:`Symfony\\Component\\DependencyInjection\\Compiler\\ServiceLocatorTagPass::register`
method to create the service locators. This will save you some boilerplate and
will share identical locators amongst all the services referencing them::

    use Symfony\Component\DependencyInjection\Compiler\ServiceLocatorTagPass;
    use Symfony\Component\DependencyInjection\ContainerBuilder;

    public function process(ContainerBuilder $container)
    {
        // ...

        $locateableServices = [
            // ...
            'logger' => new Reference('logger'),
        ];

        $myService->addArgument(ServiceLocatorTagPass::register($container, $locateableServices));
    }

.. _`Command pattern`: https://en.wikipedia.org/wiki/Command_pattern
