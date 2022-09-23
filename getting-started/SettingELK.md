### Setting up Elasticsearch Kibana and Logstash

 * You can get the pre-configured package at https://github.com/deviantony/docker-elk
    also, you can configure your system defaults also work but less securely please check  https://github.com/deviantony/docker-elk/blob/main/README.md for further information on configuration details

 ![git clone](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/git-clone-elk.png)

   Clone or download the repository
   Open Terminal inside repo and run
```console
$ sudo docker-compose up -d
```
   -d is for running at the background and if permissions are asked please re-run with sudo privileges

 ![Docker compose up](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/docker-compose-allup.png)

   You should be able to see docker containers.
    
check with
```console
sudo docker ps
```
![Docker ps](https://github.com/ComodoSecurity/openedr/blob/main/docs/screenshots/docker-ps-list.png)

   keep in mind kibana is the log ui in this setup  we gonna use kibanas port later on
    by now your monitoring tools are up and running
    Reminder default password and user would be 
     Username : elsatic
     Password : changeme
