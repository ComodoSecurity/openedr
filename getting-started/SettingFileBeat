### Setting up Openedr and File beat
 * openedr is a simple msi installer most of the current windows machines are capable you can use the installer and its own instructions.
 * Filebeat is a log analysis tool you can download the installer and follow up on its instructinos https://www.elastic.co/downloads/beats/filebeat

    First, you need to enable logstash within Filebeat to do that
    >Open Powershell as administrator
    >Change directory to within Powershell "C:\Program Files\Elastic\Beats\8.4.1\filebeat"
    > run this command
```console
.\filebeat.exe modules enable logstash --path.config"C:\ProgramData\Elastic\Beats\filebeat"
```
![Filebeat module](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/filebeat-enable-module-logstash.png)

This command will enable logstash feature and choose your configuration path

 * For filebeat configuration first go to C:\ProgramData\Elastic\Beats\filebeat
   You can check out the filebeat.example.yaml and edit that for your needs.
   and now we need to create a file as "filebeat.yaml" inside the directory.
    >Tip you can copy filebeat.example.yaml and just edit from there
   Your filebeat.yaml needs these configurations activated and edited for your IP addresses and usage
```console
   
filebeat.inputs:

-type: filestream

  id: edr

  enabled: true
     paths:

    -C:\ProgramData\edrsvc\log\*



filebeat.config.modules:
  
  path: ${path.config}/modules.d/*.yml

  reload.enabled: false
  
setup.template.settings:
  index.number_of_shards: 1

setup.kibana:

    output.logstash:
     hosts: ["Your docker adress:5044"]
```
![filebeat config](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/filebeatinputs-filebeatyaml.png)
![filebeat config](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/filebeatmodules-filebeatyaml.png)
![filebeat config](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/logstashconfig-filebeatyml.png)

* Now we have to configure activated logstash on filebeat go to  C:\ProgramData\Elastic\Beats\filebeat\modules.d\
    You can check out logstash.yaml and edit accourding to your needs and configuration.
    Edit logstash.yaml as accordingly

```console
module: logstash

  log:
    enabled: true


    var.paths:
     - C:\ProgramData\edrsvc\log\*

  slowlog:
    enabled: false
```
 ![Logstash Config](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/logstash-yaml.png)

 as for the final step to run filebeat with these configurations please restart the filebeat service from your services.msc
 or run this command from your Powershell as an administrator

![Services msc](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/services-filebeat-restart.png)

 ```console
    Restart-Service -Force filebeat
 ```
 ![Powershell Services](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/filebeat-service-restart.png)
