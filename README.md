# OpenEDR 
[![OpenEDR](https://techtalk.comodo.com/wp-content/uploads/2020/09/logo_small.jpg)](https://openedr.com/)
[![Slack](https://img.shields.io/badge/slack-join-blue.svg)](https://openedr.com/register/) [![Email](https://img.shields.io/badge/email-join-blue.svg)](mailto:register@openedr.com)

We at OpenEDR believe in creating an open source cybersecurity platform where products and services can be provisioned and managed together. EDR is our starting point.
OpenEDR is a full blown EDR capability. It is one of the most sophisticated, effective EDR code base in the world and with the community’s help it will become even better.

OpenEDR is free and open source platform  allows you to analyze what’s happening across your entire environment at base-security-event level. This granularity enables accurate root-causes analysis needed for faster and more effective remediation. Proven to be the best way to convey this type of information, process hierarchy tracking provide more than just data, they offer actionable knowledge. It collects all the details on endpoints, hashes, and base and advanced events. You get detailed file and device trajectory information and can navigate single events to uncover a larger issue that may be compromising your system.

OpenEDR’s security architecture simplifies *breach detection, protection and visibility* by working for all threat vectors without requiring any other agent or solution. The agent records all telemetry information locally and then will send the data to locally hosted or cloud hosted ElasticSeach deployments. Real-time visibility and continuous analysis are the vital elements of the entire endpoint security concept. OpenEDR enables you to perform analysis into what's happening across your environment at base event level granularity. This allows accurate root cause analysis leading to better remediation of your compromises. Integrated Security Architecture of OpenEDR delivers Full Attack Vector Visibility including MITRE Framework. 

The Open EDR consists of the following components:

* Runtime components
  * Core Library – the basic framework; 
  * Service – service application;
  * Process Monitor – components for per-process monitoring; 
    * Injected DLL – the library which is injected into different processes and hooks API calls;
    * Loader for Injected DLL – the driver component which loads injected DLL into each new process
    * Controller for Injected DLL – service component for interaction with Injected DLL;
  * System Monitor – the genetic container for different kernel-mode components;
  * File-system mini-filter – the kernel component that hooks I/O requests file system;
  * Low-level process monitoring component – monitors processes creation/deletion using system callbacks
  * Low-level registry monitoring component – monitors registry access using system callbacks
  * Self-protection provider – prevents EDR components and configuration from unauthorized changes
  * Network monitor – network filter for monitoring the network activity;
* Installer

Generic high-level interaction diagram for runtime components
![](https://techtalk.comodo.com/wp-content/uploads/2020/09/image.png)
For details you can refer here : https://techtalk.comodo.com/2020/09/19/open-edr-components/

# Community
* Community Forums: https://community.openedr.com/
* Join to Slack [![Slack](https://img.shields.io/badge/slack-join-blue.svg)](https://openedr.com/register/)
* Registration [![Email](https://img.shields.io/badge/email-join-blue.svg)](mailto:register@openedr.com)

# Build Instructions
You should have Microsoft Visual Studio to build the code
* Microsoft Visual Studio Solution File is under /openedr/edrav2/build/vs2019/
* All OpenEDR Projects are in /openedr/edrav2/iprj folder
* All external Projects and Libraries are in /openedr/edrav2/eprj folder

## Libraries Used:
* AWS SDK AWS SDK for C++ : (https://aws.amazon.com/sdk-for-cpp/)
* Boost C++ Libraries (http://www.boost.org/)
* c-ares: asynchronous resolver library (https://github.com/c-ares/c-ares)
* Catch2: a unit testing framework for C++ (https://github.com/catchorg/Catch2)
* Clare : Command Line parcer for C++ (https://github.com/catchorg/Clara)
* Cli: cross-platform header only C++14 library for interactive command line interfaces (https://cli.github.com/) 
* Crashpad: crash-reporting system (https://chromium.googlesource.com/crashpad/crashpad/+/master/README.md)
* Curl: command-line tool for transferring data specified with URL syntax (https://curl.haxx.se/)
* Detours: for monitoring and instrumenting API calls on Windows. (https://github.com/microsoft/Detours)
* Google APIs: Google APIs (https://github.com/googleapis/googleapis)
* JsonCpp: C++ library that allows manipulating JSON values (https://github.com/open-source-parsers/jsoncpp)
* libjson-rpc-cpp: cross platform JSON-RPC (remote procedure call) support for C++  (https://github.com/cinemast/libjson-rpc-cpp)
* libmicrohttpd : C library that provides a compact API and implementation of an HTTP 1.1 web server (https://www.gnu.org/software/libmicrohttpd/)
* log4cplus: C++17 logging API (https://github.com/log4cplus/log4cplus)
* MadcHook, MadcHookDrv : Hooking http://madshi.net/
* NetFilter SDK & ProtocolFilter: Network filtering toolkit (https://netfiltersdk.com/)
* nlohmann JSON: JSON library for C++: (https://github.com/nlohmann/json)
* OpenSSL Toolkit (http://www.openssl.org/)
* Tiny AES in C: implementation of the AES ECB, CTR and CBC encryption algorithms written in C. (https://github.com/kokke/tiny-AES-c)
* Uri: C++ Network URI (http://www.boost.org/)
* UTF8-CPP: UTF-8 with C++ (http://utfcpp.sourceforge.net/)
* xxhash_cpp: xxHash library to C++17. (https://cyan4973.github.io/xxHash/)
* Zlib: Compression Libraries (https://zlib.net/)

# Installation Instructions
OpenEDR is single agent that can be installed on Windows endpoints. It generates extensible telemetry data over all security relevant events. It also use file lookup, analysis and verdict systems from Comodo, https://valkyrie.comodo.com/. You can also have your own account and free license there.

The telemetry data is stored locally on the endpoint itself. You can use any log streaming solution and analysis platform. Here we will present, how can you do remote streaming and analysis via open source tools like Elasticsearch and Filebeat.

## OpenEDR :
OpenEDR project will release installer MSI’s signed by Comodo Security Solutions, The default installation folder is C:\Program Files\OpenEdr\EdrAgentV2, currently we don’t have many option to edit/configure the rule set, alerts etc.  Those will be coming with upcoming releases. 

The agent outputs to C:\ProgramData\edrsvc\log\output_events by default, there you will see the EDR telemetry data where you should point this to Filebeat or other log streaming solutions you want.

## Elasticsearch:
There are multiple options to run Elasticsearch, you can either install and run it on your own machine, on your data center or use Elasticsearch service on public cloud providers like AWS and GCP. If you want to run Elasticsearch by yourself. You can refer to here for installation instructions on various platforms https://www.elastic.co/guide/en/elasticsearch/reference/current/install-elasticsearch.html

Another option is using Docker, this will also enable a quick start for PoC and later can be extended to be production environment as well. You can access the guide for this setup here : https://www.elastic.co/guide/en/elasticsearch/reference/7.10/docker.html
## Filebeat:
Filebeat is very good option to transfer OpenEDR outputs to Elasticsearch, you need to install Filebeat on each system you want to monitor. Overall instructions for it can be found here : https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-installation-configuration.html 

We don’t have OpenEDR Filebeat modules yet so you need to configure custom input option for filebeat https://www.elastic.co/guide/en/beats/filebeat/current/configuration-filebeat-options.html


# Releases
https://github.com/ComodoSecurity/openedr/releases/tag/2.0.0.0

# Screenshots
How OpenEDR integration with a platform looks like and also a showcase for openedr capabilities

Detection / Alerting
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_1.jpg)](https://enterprise.comodo.com/dragon/)

Event Details
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_2.jpg)](https://enterprise.comodo.com/dragon/)

Dashboard
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_3.jpg)](https://enterprise.comodo.com/dragon/)

Process Timeline
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_4.jpg)](https://enterprise.comodo.com/dragon/)

Process Treeview
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_5.jpg)](https://enterprise.comodo.com/dragon/)

Event Search
[![OpenEDR](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/Screenshot_6.jpg)](https://enterprise.comodo.com/dragon/)
