{
	"description": "Record the events from System Monitor",
	"commands": [
		{
			"description": "Initialize output queue for System Monitor",
			"command": {
				"processor": "objects.application",
				"command": "putCatalogData",
				"params": {
					"path": "objects.queueCurrOutput",
					"data": {
						"$$proxy": "cachedCmd",
						"processor": "objects.queueManager",
						"command": "getQueue",
						"params": {
							"name": "inbox"
						}
					}
				}
			}
		},
		{
			"description": "Register System Monitor stop handler",
			"command": {
				"processor": "objects.application",
				"command": "subscribeToMessage",
				"params": {
					"id": "appScriptStop", // Event ID
					"command": {
						"processor": "objects.systemMonitorController",
						"command": "stop",
						"params": {}
					}
				}
			}
		},
		{
			"description": "Start events enricher",
			"command": {
				"processor": "objects.eventEnricher",
				"command": "start",
				"params": {}
			}
		},
		{
			"description": "Start System Monitor",
			"command": {
				"processor": "objects.systemMonitorController",
				"command": "start",
				"params": {}
			}
		}
	]
}