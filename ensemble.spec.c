#include <assert.h>

#include "ensemble.c"

void testSmallEnsemble() {
        ensemble_t* ensemble = ens_create(64);
        ens_add_element(ensemble, 0);
        assert(ens_contains(ensemble, 0));
        ens_add_element(ensemble, 4);
        assert(ens_contains(ensemble, 4));
        assert(!ens_contains(ensemble, 3));
        assert(!ens_contains(ensemble, 5));
        ens_destroy(ensemble);
}

void testLargeEnsemble() {
        ensemble_t* ensemble = ens_create(256);
        ens_add_element(ensemble, 200);
        assert(ens_contains(ensemble, 200));
        assert(!ens_contains(ensemble, 0));
        assert(!ens_contains(ensemble, 3));
        assert(!ens_contains(ensemble, 5));
        ens_destroy(ensemble);
}

void testFindBucket() {
        assert(ENS_ELEMENT_INDEX(0) == 0);
        assert(ENS_ELEMENT_INDEX(2) == 0);
        assert(ENS_ELEMENT_INDEX(10) == 0);
        assert(ENS_ELEMENT_INDEX(63) == 3);
        assert(ENS_ELEMENT_INDEX(64) == 4);
        assert(ENS_ELEMENT_INDEX(65) == 4);
        assert(ENS_ELEMENT_INDEX(127) == 7);
        assert(ENS_ELEMENT_INDEX(128) == 8);
}

void testBucketShift() {
        assert(ENS_ELEMENT_SHIFT(0) == 0);
        assert(ENS_ELEMENT_SHIFT(2) == 0);
        assert(ENS_ELEMENT_SHIFT(10) == 0);
        assert(ENS_ELEMENT_SHIFT(15) == 0);
        assert(ENS_ELEMENT_SHIFT(16) == 16);
        assert(ENS_ELEMENT_SHIFT(31) == 16);
        assert(ENS_ELEMENT_SHIFT(32) == 32);
}
