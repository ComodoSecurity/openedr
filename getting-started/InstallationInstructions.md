# Installation Instructions
OpenEDR is a single agent that can be installed on Windows endpoints. It generates extensible telemetry data for overall security-relevant events. It also uses file lookup, analysis, and verdict systems from Comodo, https://valkyrie.comodo.com/. You can also have your own account and free license there.

The telemetry data is stored locally on the endpoint itself. You can use any log streaming solution and analysis platform. Here we will present, how can you do remote streaming and analysis via open source tools like Elasticsearch ELK and Filebeat.

## OpenEDR:
OpenEDR project will release installer MSI’s signed by Comodo Security Solutions, The default installation folder is C:\Program Files\OpenEdr\EdrAgentV2

The agent outputs to C:\ProgramData\edrsvc\log\output_events by default, there you will see the EDR telemetry data where you should point this to Filebeat or other log streaming solutions you want. Please check following sections for details

## Docker and Docker Compose:
Docker is widely known virtualizon method for almost all environments you can use use docker for installing Elasticsearch and Log-stash to parse and more visibiltiy we used Docker to virtualize and make an easy setup process for monitoring our openedr agent. You can visit and look for more details and configure from https://github.com/docker


## Logstash:
Logstash is an open-source data ingestion tool that allows you to collect data from a variety of sources, transform it, and send it to your desired destination. With pre-built filters and support for over 200 plugins, Logstash allows users to easily ingest data regardless of the data source or type. We used Logstash to simplify our output to elasticsearch for more understandable logs and easily accessible by everyone who uses openedr and this example. You may visit and configure your own system as well https://github.com/elastic/logstash

## Elasticsearch:
There are multiple options to run Elasticsearch, you can either install and run it on your own machine, on your data center, or use Elasticsearch service on public cloud providers like AWS and GCP. If you want to run Elasticsearch by yourself. You can refer to here for installation instructions on various platforms https://www.elastic.co/guide/en/elasticsearch/reference/current/install-elasticsearch.html

Another option is using Docker, this will also enable a quick start for PoC and later can be extended to be production environment as well. You can access the guide for this setup here: https://www.elastic.co/guide/en/elasticsearch/reference/7.10/docker.html
## Filebeat:
Filebeat is a very good option to transfer OpenEDR outputs to Elasticsearch, you need to install Filebeat on each system you want to monitor. Overall instructions for it can be found here: https://www.elastic.co/guide/en/beats/filebeat/current/filebeat-installation-configuration.html 

We don’t have OpenEDR Filebeat modules yet so you need to configure a custom input option for filebeat https://www.elastic.co/guide/en/beats/filebeat/current/configuration-filebeat-options.html

## Example on the go
We used these given technologies for an even simpler solution for example and you can use these methods and configure them for your own need even use different technologies depending on your needs.

> Install docker on the device
> Clone pre-prepared ELK package
> Run docker-compose.yaml with docker-compose
> Install openedr and filebeat on the client device
> Configure filebeat to your elastic search and logstash

 All configure and installation documents are can be found in Getting Started sections
 

[Build Instructions](BuildInstructions.md)

[Docker Installation](DockerInstallation.md)

[Setting up Elasticsearch Kibana and Logstash](SettingELK.md)

[Setting up Openedr and File beat](SettingFileBeat.md)

[Editing Alerting Policies](EditingAlertingPolicies.md)

[Setting Up Kibana](SettingKibana.md)

