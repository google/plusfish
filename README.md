NOTE: This is not an officially supported Google product


```shell
 ____  _            __ _     _
|  _ \| |_   _ ___ / _(_)___| |__
| |_) | | | | / __| |_| / __| '_ \
|  __/| | |_| \__ \  _| \__ \ | | |
|_|   |_|\__,_|___/_| |_|___/_| |_|
```


## Introduction

Plusfish is a classic web application vulnerability scanner/fuzzer and
aimed at security professionals.

## Design philosophy

Typical web application scanners are tuned towards low noise and low
false positive rates. This is more user friendly but comes with the risk
of false negatives. For example, other scanners might use a long XSS payload
that can trigger a vulnerability in multiple conditions and this reduces the
amount of requests needed and reduces the intrusiveness of the scan.

The more complex a payload, the higher the chance it could get blocked
by the application which can result in a false positive.

Plusfish takes a complete opposite approach and uses as many payloads,
encodings, injection points, etc as possible. This results in significant more
traffic and you will also end up with more data to process after the scan.

Due to these characteristics; plusfish is positioned towards aiding
security professionals in their assessments by providing a lot of signals
rather than being a point, click and report tool.


## Key features

#### High performance
Despite the scanner currently being single process and single threaded; you can
reach 1000's requests per seconds when you tune the tool (and your server can
handle it) properly.

#### Highly customizable
The checks it does, scan duration, request rate, client certificate,
proxy usage, report type and pretty much the entire behavior of the
scanner can be controlled through flags and configuration files.

#### Browser independent
Does not make use of a browser/browser engine and has a full control
over its document fetching and rendering behavior. As such the behavior
of various browsers can be emulated (or considered) during security tests.

This also is a limitation at the moment because Javascript execution isn't
supported in this initial version. We do however plan on adding this.

#### Thorough security testing
The security checks are designed to bring confirmed and potential
security issues to the attention of security engineers. While we avoid
false positives, we certainly do not avoid edge case vulnerabilities
that require manual review for confirmation.

#### Generic check language
Generic security checks can be written in a powerful and structured language.
In the checks payloads, injection methods, encoding methods and response
matching behaviors can all be customized.

In fact, the majority of the security checks are currently implemented
in the configuration files. Adding one is just a matter of editing the
file and re-running the scan.

#### Finding hidden files or directories
Using the extensive wordlists it is possible to let plusfish hunt for hidden
files and directories. If found, they are also subject to the security tests.

## Current status
The tool is not finished and currently best for testing server-side security
problems.

## Things we want to add

#### LARGE: Javascript execution (v8)
We will add a different HTML parser with v8 dom bindings in order to better
support scans of Javascript heavy web sites.

#### SMALL: HTML reporting
Nobody likes reading XML. Adding an HTML report option will be added soon after
the public release of this project.

#### MEDIUM: Refactor all tests
Not all tests in the opensource repo can be build and run. We intend to fix this
soon after the public release because it is crucial for making the GitHub repo
our main development repo.

#### MEDIUM: More security tests
The currently provided security tests are not complete and this is an area where
security enthusiasts in the community can really step in.  PR's for new security
checks are welcome!

## More information
Please have a look at:

* The [build document](BUILD.md) on how to compile plusfish.
* The [examples document](EXAMPLES.md) for example on running a scan
* The [configuration](CONFIGURATION.md) document for insight in how security
* How to [contribute](CONTRIBUTING.md).

## Credits
Written by Niels Heinen with the help of [attwad](https://github.com/attwad)


