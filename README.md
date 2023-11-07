# OpenEDR 
[![OpenEDR](https://techtalk.comodo.com/wp-content/uploads/2020/09/logo_small.jpg)](https://openedr.com/)
[![Slack](https://img.shields.io/badge/slack-join-blue.svg)](https://openedr.com/register/) [![Email](https://img.shields.io/badge/email-join-blue.svg)](mailto:register@openedr.com)

[![OpenEDR - Getting Started](https://techtalk.comodo.com/wp-content/uploads/2022/10/Screenshot_3.jpg)](https://www.youtube.com/watch?v=lfo_fyinvYs "OpenEDR - Getting Started")    

We at OpenEDR believe in creating a cybersecurity platform with its source code openly available to the public,  where products and services can be provisioned and managed together. EDR is our starting point.
OpenEDR is a full-blown EDR capability. It is one of the most sophisticated, effective EDR code base in the world and with the community’s help, it will become even better.

OpenEDR is free and its source code is open to the public. OpenEDR allows you to analyze what’s happening across your entire environment at the base-security-event level. This granularity enables accurate root-causes analysis needed for faster and more effective remediation. Proven to be the best way to convey this type of information, process hierarchy tracking provides more than just data, they offer actionable knowledge. It collects all the details on endpoints, hashes, and base and advanced events. You get detailed file and device trajectory information and can navigate single events to uncover a larger issue that may be compromising your system.

OpenEDR’s security architecture simplifies *breach detection, protection, and visibility* by working for all threat vectors without requiring any other agent or solution. The agent records all telemetry information locally and then will send the data to locally hosted or cloud-hosted ElasticSearch deployments. Real-time visibility and continuous analysis are vital elements of the entire endpoint security concept. OpenEDR enables you to perform analysis into what's happening across your environment at base event level granularity. This allows accurate root cause analysis leading to better remediation of your compromises. Integrated Security Architecture of OpenEDR delivers Full Attack Vector Visibility including MITRE Framework. 

## Quick Start
The community response to OpenEDR has been absolutely amazing! Thank you. We had a lot of requests from people who want to deploy and use OpenEDR easily and quickly. We have a roadmap to achieve all these. However in the meanwhile, we have decided to use the Comodo Dragon Enterprise platform with OpenEDR to achieve that. By simply opening an account, you will be able to use OpenEDR. No custom installation, no log forwarding configuration, or worrying about storing telemetry data. All of that is handled by the Comodo Dragon Platform. This is only a short-term solution until all the easy-to-use packages for OpenEDR is finalized. In the meanwhile do take advantage of this by emailing quick-start@openedr.com to get you up and running!


## Components
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

For details, you can refer here: https://techtalk.comodo.com/2020/09/19/open-edr-components/

# Community
* Community Forums: https://community.openedr.com/
* Join Slack [![Slack](https://img.shields.io/badge/slack-join-blue.svg)](https://openedr.com/register/)
* Registration [![Email](https://img.shields.io/badge/email-join-blue.svg)](mailto:register@openedr.com)

# Roadmap
Please refer here for project roadmap : https://github.com/ComodoSecurity/openedr_roadmap/projects/1

## Getting Started

Please take a look at the following documents.

1. [Getting Started](getting-started/InstallationInstructions.md)
2. [Build Instructions](getting-started/BuildInstructions.md)
3. [Docker Installation](getting-started/DockerInstallation.md)
4. [Setting up Elasticsearch Kibana and Logstash](getting-started/SettingELK.md)
5. [Setting up Openedr and File beat](getting-started/SettingFileBeat.md)
6. [Editing Alerting Policies](getting-started/EditingAlertingPolicies.md)
7. [Setting Up Kibana](getting-started/SettingKibana.md)

 
 # Releases
https://github.com/ComodoSecurity/openedr/releases/tag/release-2.5.1
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
