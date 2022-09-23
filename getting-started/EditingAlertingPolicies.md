## Editing Alerting Policies 
The agent uses network driver, file driver, and DLL injection to capture events that occur on the endpoint. It enriches the event data with various information, then filters these events according to the policy rules and sends them to the server. 

You can customize your policy with your own policy. Within the installation folder which is "C:\Program Files\Comodo\EdrAgentV2" policy file called   "evm.local.src"

For Comodo suggested rules please check the rule repo https://github.com/ComodoSecurity/OpenEDRRules

You can edit this file with any text editor and customize your own policy accordingly with the given information below.
```console
{
    "Lists": {
        "List1": ["string1", "string2", ...],
        "List2": ["string1", "string2", ...],
        ...
    },
    "Events": {
        "event_code": [<AdaptiveEvent1>, <AdaptiveEvent2>, ...],  --> The order of the adaptive events in the list is important (see Adaptive Event Ordering).
        "event_code": [<AdaptiveEvent1>, <AdaptiveEvent2>, ...],
        ...
    }
}
```

### Adaptive Event
```console
{
    "BaseEventType": 3
    "EventType": null|GUID,
    "Condition": <Condition>
}
```

### Condition
```console
{
    "Field": "parentVerdict",
    "Operator": "!Equal",
    "Value": 1
}
```
Boolean operators are supported. The following also is a condition:
```console
Equal",
    "Value": 1
}
```
Boolean operators are supported. The following is also a condition:
```console
{
    "BooleanOperator": "Or",
    "Conditions": [
        {
            "Field": "parentVerdict",
            "Operator": "!Equal",
            "Value": 1
        },
        {
            "Field": "parentProcessPath",
            "Operator": "MatchInList",
            "Value": "RegWhiteList"
        }
    ]
}
```
Nesting with Boolean operators are supported. The following is also a condition:
```console
{
    "BooleanOperator": "And",
    "Conditions": [
        {
            "Field": "parentProcessPath",
            "Operator": "!Match",
            "Value": "*\\explorer.exe"
        },
        {
            "BooleanOperator": "Or",
            "Conditions": [
                {
                    "Field": "path",
                    "Operator": "Match",
                    "Value": "*\\powershell.exe"
                },
                {
                    "Field": "path",
                    "Operator": "Match",
                    "Value": "*\\cmd.exe"
                }
            ]
        }
    ]
}

```
The BooleanOperator field can be one of the following:

>and

>or

The Operator field can be one of the following:
| Operator | Value Type | Output |
| --- | --- | --- |
| `Equal` | Number, String, Boolean, null | true if the field and the value are equal|
| `Match`| String (wildcard pattern)| true if the field matches the pattern (environment variables in the pattern must be expanded first)|
| `MatchInList` | String (list name) | true if the field matches one of the patterns in the list (environment variables in patterns must be expanded first)|


!Equal, !Match and !MatchInList must also be supported.

### Adaptive Event Ordering
When an event is captured, the conditions in the adaptive events are checked sequentially. When a matching condition is found, the BaseEventType and EventType in that adaptive event are added to the event data and the event is sent. Other conditions are not checked. No logging will be done if no advanced event conditions match. If the Condition field is not provided, it is assumed that the condition matches.
