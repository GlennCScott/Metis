/*
 * Copyright (c) 2013-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Patent rights are not granted under this agreement. Patent rights are
 *       available under FRAND terms.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Framework.
#include "../metis_FibEntry.c"
#include <LongBow/unit-test.h>
#include <parc/algol/parc_SafeMemory.h>

LONGBOW_TEST_RUNNER(metis_FibEntry)
{
    // The following Test Fixtures will run their corresponding Test Cases.
    // Test Fixtures are run in the order specified, but all tests should be idempotent.
    // Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(metis_FibEntry)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(metis_FibEntry)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_AddNexthop);
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_Create_Destroy);
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_GetNexthops);
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_NexthopCount);
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_RemoveNexthop);
    LONGBOW_RUN_TEST_CASE(Global, metisFibEntry_SetStrategy);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, metisFibEntry_AddNexthop)
{
    CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/foo/bar");
    MetisTlvName *tlvName = metisTlvName_CreateFromCCNxName(ccnxName);
    MetisFibEntry *fibEntry = metisFibEntry_Create(tlvName);

    metisFibEntry_AddNexthop(fibEntry, 1);
    metisFibEntry_AddNexthop(fibEntry, 2);

    assertTrue(metisNumberSet_Length(fibEntry->nexhops) == 2, "wrong nexthop length, expected %u got %zu", 2, metisNumberSet_Length(fibEntry->nexhops));
    assertTrue(metisNumberSet_Contains(fibEntry->nexhops, 1), "wrong nexthops, did not contain %u", 1);
    assertTrue(metisNumberSet_Contains(fibEntry->nexhops, 2), "wrong nexthops, did not contain %u", 2);

    metisFibEntry_Release(&fibEntry);
    metisTlvName_Release(&tlvName);
    ccnxName_Release(&ccnxName);
}

LONGBOW_TEST_CASE(Global, metisFibEntry_Create_Destroy)
{
    CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/foo/bar");
    MetisTlvName *tlvName = metisTlvName_CreateFromCCNxName(ccnxName);

    size_t beforeMemory = parcMemory_Outstanding();
    MetisFibEntry *fibEntry = metisFibEntry_Create(tlvName);
    metisFibEntry_Release(&fibEntry);
    size_t afterMemory = parcMemory_Outstanding();

    metisTlvName_Release(&tlvName);
    ccnxName_Release(&ccnxName);

    assertTrue(beforeMemory == afterMemory, "Memory imbalance on create/destroy: expected %zu got %zu", beforeMemory, afterMemory);
}

LONGBOW_TEST_CASE(Global, metisFibEntry_GetNexthops)
{
    CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/foo/bar");
    MetisTlvName *tlvName = metisTlvName_CreateFromCCNxName(ccnxName);
    MetisFibEntry *fibEntry = metisFibEntry_Create(tlvName);

    metisFibEntry_AddNexthop(fibEntry, 1);
    metisFibEntry_AddNexthop(fibEntry, 2);

    const MetisNumberSet *nexthops = metisFibEntry_GetNexthops(fibEntry);
    assertTrue(metisNumberSet_Equals(nexthops, fibEntry->nexhops), "did not returns the right set");

    metisFibEntry_Release(&fibEntry);
    metisTlvName_Release(&tlvName);
    ccnxName_Release(&ccnxName);
}

LONGBOW_TEST_CASE(Global, metisFibEntry_NexthopCount)
{
    CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/foo/bar");
    MetisTlvName *tlvName = metisTlvName_CreateFromCCNxName(ccnxName);
    MetisFibEntry *fibEntry = metisFibEntry_Create(tlvName);

    metisFibEntry_AddNexthop(fibEntry, 1);
    metisFibEntry_AddNexthop(fibEntry, 2);

    assertTrue(metisFibEntry_NexthopCount(fibEntry) == 2, "Returned wrong number of next hops, expected %u got %zu", 2, metisFibEntry_NexthopCount(fibEntry));

    metisFibEntry_Release(&fibEntry);
    metisTlvName_Release(&tlvName);
    ccnxName_Release(&ccnxName);
}

LONGBOW_TEST_CASE(Global, metisFibEntry_RemoveNexthop)
{
    CCNxName *ccnxName = ccnxName_CreateFromURI("lci:/foo/bar");
    MetisTlvName *tlvName = metisTlvName_CreateFromCCNxName(ccnxName);
    MetisFibEntry *fibEntry = metisFibEntry_Create(tlvName);

    metisFibEntry_AddNexthop(fibEntry, 1);
    metisFibEntry_AddNexthop(fibEntry, 2);

    metisFibEntry_RemoveNexthop(fibEntry, 2);
    assertTrue(metisFibEntry_NexthopCount(fibEntry) == 1, "Returned wrong number of next hops, expected %u got %zu", 1, metisFibEntry_NexthopCount(fibEntry));
    assertTrue(metisNumberSet_Contains(fibEntry->nexhops, 1), "wrong nexthops, did not contain %u", 1);

    metisFibEntry_Release(&fibEntry);
    metisTlvName_Release(&tlvName);
    ccnxName_Release(&ccnxName);
}

LONGBOW_TEST_CASE(Global, metisFibEntry_SetStrategy)
{
    testUnimplemented("This test is unimplemented");
}

LONGBOW_TEST_FIXTURE(Local)
{
}

LONGBOW_TEST_FIXTURE_SETUP(Local)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Local)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(metis_FibEntry);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
