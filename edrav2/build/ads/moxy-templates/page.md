# {{kind}} `{{name}}`

{{briefdescription}}

{{detaileddescription}}

{{#if filtered.members}}

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
{{#each filtered.members}}{{#if summary}}{{cell proto}}            | {{cell summary}}
{{/if}}{{/each}}{{#each filtered.compounds}}{{#if summary}}{{cell proto}} | {{cell summary}}
{{/if}}{{/each}}

## Members

{{#each filtered.members}}
{{#if summary}}
### {{title proto}} {{anchor refid}}

{{#if enumvalue}}
 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
{{#each enumvalue}}{{#if summary}}{{cell name}}            | {{cell summary}}
{{/if}}{{/each}}
{{/if}}

{{briefdescription}}

{{detaileddescription}}

{{/if}}

{{/each}}
{{/if}}
