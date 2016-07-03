#include "../../common.h"

#include "../../ds/ds_htable.h"
#include "php_htable_iterator.h"

static inline ds_htable_bucket_t *find_starting_bucket(ds_htable_t *h)
{
    ds_htable_bucket_t *b = h->buckets;

    if (h->size != 0) {
        ds_htable_bucket_t *m = h->buckets + h->capacity;

        while (b != m && DS_HTABLE_BUCKET_DELETED(b)) {
            ++b;
        }
    }

    return b;
}


static void iterator_dtor(zend_object_iterator *i)
{
    DTOR_AND_UNDEF(&i->data);
}

static int iterator_valid(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;
    uint32_t size             = iterator->h->size;
    uint32_t position         = iterator->position;

    return position < size ? SUCCESS : FAILURE;
}

static zval *iterator_get_current_value(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    ds_htable_bucket_t *bucket = iterator->b;

    if (DS_HTABLE_BUCKET_NOT_DELETED(bucket)) {
        return &bucket->value;
    }

    return NULL;
}

static zval *iterator_get_current_keyval(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    ds_htable_bucket_t *bucket = iterator->b;

    if (DS_HTABLE_BUCKET_NOT_DELETED(bucket)) {
        return &bucket->key;
    }

    return NULL;
}

static void iterator_get_current_key(zend_object_iterator *i, zval *key)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    ds_htable_bucket_t *bucket = iterator->b;

    if (DS_HTABLE_BUCKET_NOT_DELETED(bucket)) {
        ZVAL_COPY(key, &bucket->key);
    }
}

static zval *iterator_get_current_pair(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    ds_htable_bucket_t *bucket = iterator->b;

    if (DS_HTABLE_BUCKET_NOT_DELETED(bucket)) {

        zval *key = &bucket->key;
        zval *val = &bucket->value;

        zval *arr = &iterator->intern.data;

        Z_TRY_ADDREF_P(key);
        Z_TRY_ADDREF_P(val);

        array_init_size(arr, 2);

        add_next_index_zval(arr, key);
        add_next_index_zval(arr, val);

        return arr;
    }

    return NULL;
}

static void iterator_get_current_pos(zend_object_iterator *i, zval *key)
{
    ZVAL_LONG(key, ((ds_htable_tIterator *) i)->position);
}

static void iterator_move_forward(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    if (++iterator->position < iterator->h->size) {
        do {
            ++iterator->b;
        } while (DS_HTABLE_BUCKET_DELETED(iterator->b));
    }
}

static void iterator_rewind(zend_object_iterator *i)
{
    ds_htable_tIterator *iterator = (ds_htable_tIterator *) i;

    iterator->position = 0;
    iterator->b = find_starting_bucket(iterator->h);
}

static zend_object_iterator_funcs ds_htable_get_value_iterator_funcs = {
    iterator_dtor,
    iterator_valid,
    iterator_get_current_value, // value
    iterator_get_current_pos,   // key
    iterator_move_forward,
    iterator_rewind
};

static zend_object_iterator_funcs ds_htable_get_key_iterator_funcs = {
    iterator_dtor,
    iterator_valid,
    iterator_get_current_keyval, // value
    iterator_get_current_pos,    // key
    iterator_move_forward,
    iterator_rewind
};

static zend_object_iterator_funcs ds_htable_get_pair_iterator_funcs = {
    iterator_dtor,
    iterator_valid,
    iterator_get_current_pair, // value
    iterator_get_current_pos,  // key
    iterator_move_forward,
    iterator_rewind
};

static zend_object_iterator_funcs ds_htable_get_assoc_iterator_funcs = {
    iterator_dtor,
    iterator_valid,
    iterator_get_current_value, // value
    iterator_get_current_key,   // key
    iterator_move_forward,
    iterator_rewind
};


static zend_object_iterator *create_htable_iterator(ds_htable_t *h, zend_object_iterator_funcs *funcs, int by_ref)
{
    ds_htable_tIterator *iterator;

    if (by_ref) {
        ITERATION_BY_REF_NOT_SUPPORTED();
        return NULL;
    }

    iterator = ecalloc(1, sizeof(ds_htable_tIterator));

    zend_iterator_init((zend_object_iterator*) iterator);

    ZVAL_UNDEF(&iterator->intern.data);

    iterator->intern.funcs  = funcs;
    iterator->h             = h;

    return (zend_object_iterator *) iterator;
}

zend_object_iterator *ds_htable_get_value_iterator_ex(zend_class_entry *ce, zval *obj, int by_ref, ds_htable_t *h)
{
    return create_htable_iterator(h, &ds_htable_get_value_iterator_funcs, by_ref);
}

zend_object_iterator *ds_htable_get_key_iterator_ex(zend_class_entry *ce, zval *obj, int by_ref, ds_htable_t *h)
{
    return create_htable_iterator(h, &ds_htable_get_key_iterator_funcs, by_ref);
}

zend_object_iterator *ds_htable_get_pair_iterator_ex(zend_class_entry *ce, zval *obj, int by_ref, ds_htable_t *h)
{
    return create_htable_iterator(h, &ds_htable_get_pair_iterator_funcs, by_ref);
}

zend_object_iterator *ds_htable_get_assoc_iterator_ex(zend_class_entry *ce, zval *obj, int by_ref, ds_htable_t *h)
{
    return create_htable_iterator(h, &ds_htable_get_assoc_iterator_funcs, by_ref);
}

// zend_object_iterator *ds_htable_get_value_iterator(zend_class_entry *ce, zval *obj, int by_ref)
// {
//     ds_htable_t *h = HTABLE_ITERATOR_FROM_ZVAL(obj)->h;
//     return create_htable_iterator(h, &htable_get_value_iterator_funcs, by_ref);
// }

// zend_object_iterator *ds_htable_get_key_iterator(zend_class_entry *ce, zval *obj, int by_ref)
// {
//     ds_htable_t *h = HTABLE_ITERATOR_FROM_ZVAL(obj)->h;
//     return create_htable_iterator(h, &htable_get_key_iterator_funcs, by_ref);
// }

// zend_object_iterator *ds_htable_get_pair_iterator(zend_class_entry *ce, zval *obj, int by_ref)
// {
//     ds_htable_t *h = HTABLE_ITERATOR_FROM_ZVAL(obj)->h;
//     return create_htable_iterator(h, &htable_get_pair_iterator_funcs, by_ref);
// }

// zend_object_iterator *ds_htable_get_assoc_iterator(zend_class_entry *ce, zval *obj, int by_ref)
// {
//     ds_htable_t *h = HTABLE_ITERATOR_FROM_ZVAL(obj)->h;
//     return create_htable_iterator(h, &htable_get_assoc_iterator_funcs, by_ref);
// }
