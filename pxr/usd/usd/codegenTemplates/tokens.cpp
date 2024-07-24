//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "{{ libraryPath }}/tokens.h"

{% if useExportAPI %}
{{ namespaceOpen }}

{% endif %}
{{ tokensPrefix }}TokensType::{{ tokensPrefix }}TokensType() :
{% for token in tokens %}
    {{ token.id }}("{{ token.value }}", TfToken::Immortal),
{% endfor %}
    allTokens({
{% for token in tokens %}
        {{ token.id }}{% if not loop.last %},{% endif %}

{% endfor %}
    })
{
}

TfStaticData<{{ tokensPrefix }}TokensType> {{ tokensPrefix }}Tokens;
{% if useExportAPI %}

{{ namespaceClose }}
{% endif %}
