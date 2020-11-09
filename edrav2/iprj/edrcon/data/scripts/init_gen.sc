{
	"description": "Initialize random data generator",
	"commands": [
		{
			"description": "Create random data generator",
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
			"description": "Create constants dictionary",
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
		}
	]
}
