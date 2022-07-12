# {{kind}} `{{name}}`

{{#if basecompoundref}}
## Parents

  {{#each basecompoundref}}
* **{{prot}} {{name}}**
  {{/each}}
  
{{/if}}

## Description

{{briefdescription}}

{{detaileddescription}}

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
{{#each filtered.compounds}}{{#if summary}}{{cell proto}}        | {{cell summary}}
{{/if}}{{/each}}{{#each filtered.members}}{{#if summary}}{{cell proto}} | {{cell summary}}
{{/if}}{{/each}}

## Members

{{#each filtered.compounds}}
### {{title proto}} {{anchor refid}}

{{briefdescription}}

{{detaileddescription}}
{{/each}}

{{#each filtered.members}}
{{#if summary}}

### {{title name}} {{anchor refid}}
__{{proto}}__

{{#if enumvalue}}
 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
{{#each enumvalue}}{{cell name}}            | {{cell summary}}
{{/each}}
{{/if}}

{{briefdescription}}

{{detaileddescription}}

{{#if inbodydescription}}
{{inbodydescription}}
{{/if}}

{{/if}}
{{/each}}
