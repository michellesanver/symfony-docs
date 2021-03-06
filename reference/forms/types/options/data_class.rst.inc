data_class
~~~~~~~~~~

**type**: ``string``

This option is used to set the appropriate data mapper to be used by the
form, so you can use it for any form field type which requires an object::

    use AppBundle\Entity\Media;
    use AppBundle\Form\MediaType;
    // ...

    $builder->add('media', MediaType::class, [
        'data_class' => Media::class,
    ]);
