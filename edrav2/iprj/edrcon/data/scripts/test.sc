{
  "description": "This is the test scenario",
  "reporter": "objects.gen01", // The name of the IReporterProvider object
  "commands": [
    {
      "description": "Creating random data generator",
      "command": {
        "processor": "objects.application",
        "command": "putCatalogData",
        "params": {
          "path": "objects.rnd01",
          "data": {
            "$$clsid": "0xD3A70691", // CLSID_RandomDataGenerator
            "start": 0 // Start value for generation (0 by default)
          }
        }
      }
    },
    {
      "description": "Creating constants dictionary",
      "command": {
        "processor": "objects.application",
        "command": "putCatalogData",
        "params": {
          "path": "objects.const01",
          "data": {
            "deviceName": "TEST-PC", // CLSID_RandomDataGenerator
            "zeroTime": {
              "$$proxy": "cachedCmd",
              "processor": "objects.rnd01",
              "command": "getIsoDateTime",
              "params": { "shift": -3000 } // Shift from current time in milliseconds (0 by default)
            }
          }
        }
      }
    },
    {
      "description": "Creating data generator",
      "command": {
        "processor": "objects.application",
        "command": "putCatalogData",
        "params": {
          "path": "objects.gen01",
          "data": {
            "$$clsid": "0xB769174E", // CLSID_DataGenerator
            "period": 500,
            "data": {
							"default": {
								"$$clsid": "0xDA024E57", // CLSID_FsDataStorage
								"path": "test.data"
							}
            },
						"receiver": {
							"$$clsid": "0xCEFC4454", // CLSID_File
							"path": "test.sc.out",
							"putDelimiter": "\n\n",
							"mode": "wtRW"
						}
          }
        }
      }
    },
    {
      "description": "Starting FS-based data generator",
      "command": {
        "processor": "objects.gen01",
        "command": "start",
        "params": {
          "count": {
            "$$proxy": "cachedCmd",
            "processor": "objects.application",
            "command": "getCatalogData",
            "params": {
              "path": "app.params.script.count",
              "default": 100000
            }
          }
        }
      }
    }
  ]
}
