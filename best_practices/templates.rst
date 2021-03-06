Templates
=========

When PHP was created 20 years ago, developers loved its simplicity and how
well it blended HTML and dynamic code. But as time passed, other template
languages - like `Twig`_ - were created to make templating even better.

.. best-practice::

    Use Twig templating format for your templates.

Generally speaking, PHP templates are much more verbose than Twig templates because
they lack native support for lots of modern features needed by templates,
like inheritance, automatic escaping and named arguments for filters and
functions.

Twig is the default templating format in Symfony and has the largest community
support of all non-PHP template engines (it's used in high profile projects
such as Drupal 8).

In addition, Twig is the only template format with guaranteed support in Symfony
3.0. As a matter of fact, PHP may be removed from the officially supported
template engines.

Template Locations
------------------

.. best-practice::

    Store all your application's templates in ``app/Resources/views/`` directory.

Traditionally, Symfony developers stored the application templates in the
``Resources/views/`` directory of each bundle. Then they used the Twig namespaced
path to refer to them (e.g. ``@AcmeDemo/Default/index.html.twig``).

But for the templates used in your application, it's much more convenient
to store them in the ``app/Resources/views/`` directory. For starters, this
drastically simplifies their logical names:

============================================  ==================================
Templates Stored inside Bundles               Templates Stored in ``app/``
============================================  ==================================
``@AcmeDemo/index.html.twig``                 ``index.html.twig``
``@AcmeDemo/Default/index.html.twig``         ``default/index.html.twig``
``@AcmeDemo/Default/subdir/index.html.twig``  ``default/subdir/index.html.twig``
============================================  ==================================

Another advantage is that centralizing your templates simplifies the work
of your designers. They don't need to look for templates in lots of directories
scattered through lots of bundles.

.. best-practice::

    Use lowercased snake_case for directory and template names.

.. best-practice::

    Use a prefixed underscore for partial templates in template names.

You often want to reuse template code using the ``include`` function to avoid
redundant code. To determine those partials in the filesystem you should
prefix partials and any other template without HTML body or ``extends`` tag
with a single underscore.

Twig Extensions
---------------

.. best-practice::

    Define your Twig extensions in the ``AppBundle/Twig/`` directory. Your
    application will automatically detect them and configure them.

Our application needs a custom ``md2html`` Twig filter so that we can transform
the Markdown contents of each post into HTML.

To do this, first, install the excellent `Parsedown`_ Markdown parser as
a new dependency of the project:

.. code-block:: terminal

    $ composer require erusev/parsedown

Then, create a new ``Markdown`` class that will be used later by the Twig
extension. It just needs to define one single method to transform
Markdown content into HTML::

    namespace AppBundle\Utils;

    class Markdown
    {
        private $parser;

        public function __construct()
        {
            $this->parser = new \Parsedown();
        }

        public function toHtml($text)
        {
            return $this->parser->text($text);
        }
    }

Next, create a new Twig extension and define a new filter called ``md2html``
using the ``Twig\TwigFilter`` class. Inject the newly defined ``Markdown``
class in the constructor of the Twig extension::

    namespace AppBundle\Twig;

    use AppBundle\Utils\Markdown;
    use Twig\Extension\AbstractExtension;
    use Twig\TwigFilter;

    class AppExtension extends AbstractExtension
    {
        private $parser;

        public function __construct(Markdown $parser)
        {
            $this->parser = $parser;
        }

        public function getFilters()
        {
            return [
                new TwigFilter(
                    'md2html',
                    [$this, 'markdownToHtml'],
                    ['is_safe' => ['html'], 'pre_escape' => 'html']
                ),
            ];
        }

        public function markdownToHtml($content)
        {
            return $this->parser->toHtml($content);
        }

        public function getName()
        {
            return 'app_extension';
        }
    }

And that's it!

If you're using the :ref:`default services.yml configuration <service-container-services-load-example>`,
you're done! Symfony will automatically know about your new service and tag it to
be used as a Twig extension.

----

Next: :doc:`/best_practices/forms`

.. _`Twig`: https://twig.symfony.com/
.. _`Parsedown`: http://parsedown.org/
