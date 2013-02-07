# Curl Sandbox Test

## Purpose

To determine a decent way of handling a large volume of libcurl POSTs that get
pulled from a list in a multithreaded application.

## Goals

Slam the host who's our target (localhost for now) but with minimal resources
consumed by the client (our app).  Reuse curl easy_handles when possible to
reuse existing open connections.
