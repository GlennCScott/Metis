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
//
//  test_metis_CommandLineInterface.c
//  Metis
//
//  Created by Mosko, Marc <Marc.Mosko@parc.com> on 12/3/13.
//  Copyright 2013-2015 __MyCompanyName__. All rights reserved.
//

#include "../metis_CommandLineInterface.c"

#include <errno.h>
#include <string.h>

#include <LongBow/unit-test.h>
#include <LongBow/debugging.h>

#include <parc/algol/parc_SafeMemory.h>

LONGBOW_TEST_RUNNER(test_metis_CommandLineInterface)
{
// The following Test Fixtures will run their corresponding Test Cases.
// Test Fixtures are run in the order specified, but all tests should be idempotent.
// Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(test_metis_CommandLineInterface)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(test_metis_CommandLineInterface)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, myTest);
    LONGBOW_RUN_TEST_CASE(Global, Version);
}

typedef struct test_state {
    MetisForwarder *metis;
    MetisDispatcher *dispatcher;
    MetisCommandLineInterface *cli;

    int clientFd;
} TestState;

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    TestState *state = malloc(sizeof(TestState));

    state->metis = metisForwarder_Create(NULL);
    state->dispatcher = metisForwarder_GetDispatcher(state->metis);

// we create our own CLI, because the one built in to metisForwarder is not started
// until the forwarder is running.

    state->cli = metisCommandLineInterface_Create(state->metis, 2001);
    metisCommandLineInterface_Start(state->cli);

    metisDispatcher_RunCount(state->dispatcher, 1);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(2001);
    inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr));

    state->clientFd = socket(PF_INET, SOCK_STREAM, 0);
    assertFalse(state->clientFd < 0, "Error on socket: (%d) %s", errno, strerror(errno));

    int failure = connect(state->clientFd, (struct sockaddr *) &addr, sizeof(addr));
    assertFalse(failure, "Error on connect: (%d) %s", errno, strerror(errno));

// crank the handle once
    metisDispatcher_RunDuration(metisForwarder_GetDispatcher(state->metis), &((struct timeval) { 0, 1000 }));

    longBowTestCase_SetClipBoardData(testCase, state);

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    TestState *state = longBowTestCase_GetClipBoardData(testCase);

    close(state->clientFd);
    metisCommandLineInterface_Destroy(&state->cli);
    metisForwarder_Destroy(&state->metis);
    free(state);

    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

/**
 * The CLI has a secret command "~~" (two of them) that will echo back whatever the next
 * words are.  The string "~~ hello world" would echo back "success: hello world" followed by
 * the next command prompt.  This lets us test that the 1st level of parsing is working.  It
 * differentiates "~~" as the command and the rest of the string as parameters.
 */
LONGBOW_TEST_CASE(Global, myTest)
{
    TestState *state = longBowTestCase_GetClipBoardData(testCase);


    char readbuffer[1024];

// Skipover the MOTD
    ssize_t nread = read(state->clientFd, readbuffer, 1024);
    assertTrue(nread > -1, "Error read");
    printf("read:\n%s\n", readbuffer);

    // send special command "~~" followed by a string.  It should be repeated back
    // as "success: see no hands\nmetis> ", where the stuff after the \n is the next command prompt
    char magic[] = "~~ see no hands\r\n";
    ssize_t nwritten = write(state->clientFd, magic, sizeof(magic));
    assertTrue(nwritten == sizeof(magic), "Error write, expected %zu got %zd", sizeof(magic), nwritten);

    metisDispatcher_RunDuration(state->dispatcher, &((struct timeval) { 0, 1000 }));

    memset(readbuffer, 0, 1024);
    nread = read(state->clientFd, readbuffer, 1024);
    assertTrue(nread > -1, "Error read");

    // we look for the answer without the "\nmetis> " part.
    char answer[] = "success: see no hands";
    assertTrue(strncasecmp(readbuffer, answer, sizeof(answer) - 1) == 0, "Got wrong string: %s", readbuffer);
}

LONGBOW_TEST_CASE(Global, Version)
{
    TestState *state = longBowTestCase_GetClipBoardData(testCase);

    char readbuffer[1024];

    // Skipover the MOTD
    ssize_t nread = read(state->clientFd, readbuffer, 1024);
    assertTrue(nread > -1, "Error read");

    printf("read:\n%s\n", readbuffer);

    // send special command "~~" followed by a string.  It should be repeated back
    // as "success: see no hands\nmetis> ", where the stuff after the \n is the next command prompt
    char magic[] = "ver\r\n";
    ssize_t nwritten = write(state->clientFd, magic, sizeof(magic));
    assertTrue(nwritten == sizeof(magic), "Error write, expected %zu got %zd", sizeof(magic), nwritten);

    metisDispatcher_RunDuration(state->dispatcher, &((struct timeval) { 0, 1000 }));

    memset(readbuffer, 0, 1024);
    nread = read(state->clientFd, readbuffer, 1024);
    assertTrue(nread > -1, "Error read");

    printf("%s", readbuffer);
}

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(test_metis_CommandLineInterface);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
