(function() {
  'use strict';
  
  function quote_attr(str) {
    return '"' + str.replace('"', '\\"') + '"';
  };

  function build_version_select(release, all_versions) {
    var buf = ['<select id="version_select">'];
    var major_minor = release.split(".").slice(0, 2).join(".");

    $.each(all_versions, function(index, version_obj) {
      if (version_obj.version == major_minor) {
        buf.push('<option value=' + quote_attr(version_obj.tag) + ' selected="selected">' 
                 + version_obj.displayName + '</option>');
      }
      else {
        buf.push('<option value=' + quote_attr(version_obj.tag) + '>' 
                 + version_obj.displayName + '</option>');
      }
    });

    buf.push('</select>');
    return buf.join('');
  }

  var version_regexs = [
    '(?:\\d\\d.\\d\\d)',
    'dev',
    'release'
  ];

  // Returns the path segment of the version as a string, like '21.11'
  // or '' if not found.
  function version_segment_from_url() {
    var path = window.location.pathname;
    var version_segment = '(?:(?:' + version_regexs.join('|') + ')/)';
    var version_regexp = version_segment
    var match = path.match(version_regexp);
    if (match !== null) {
      return match[0];
    }
    return ''
  }

  function navigate_to_first_existing(urls) {
    // Navigate to the first existing URL in urls.
    var url = urls.shift();
    if (urls.length == 0 || url.startsWith("file:///")) {
      window.location.href = url;
      return;
    }
    $.ajax({
      url: url,
      success: function() {
        window.location.href = url;
      },
      error: function() {
        navigate_to_first_existing(urls);
      }
    });
  }

  function switch_to_tag(tag) {
    var url = window.location.href;
    var current_version = version_segment_from_url();
    var new_url = url.replace('/' + current_version,
                              '/' + tag + '/');
    if (new_url != url) {
      navigate_to_first_existing([
        new_url,
        '/' + tag + '/',
        '/'
      ]);
    }
  }

  function on_version_switch() {
    var selected_tag = $(this).children('option:selected').attr('value');
    switch_to_tag(selected_tag);
  };

  function create_version_object(entry) {
    // For convenience, we allow the .json entry to be a single string like
    // "21.08", which we'll use for all elements in the version object.
    if (typeof entry == 'string') {
      return {
        version : entry,
        displayName : entry,
        tag : entry 
      };
    }
      
    // Assume the version number is always present and use it to fill in
    // any missing properties, again for convenience.
    entry.displayName = entry.displayName || entry.version;
    entry.tag = entry.tag || entry.version;
    return entry;
  }

  $(document).ready(function() {

    // Dynamically build version selector from manifest of available versions
    // stored in versions.json file. This file is expected to contain a
    // "versions" array listing versions in the order they should be displayed
    // in the version selector. 
    //
    // Each element in the "versions" array should either be a dictionary
    // containing a version number and other information, or just a string
    // version number. The version number will be used to fill in any missing
    // properties. For example:
    //
    // {
    //    "versions" : [
    //        {
    //            "version" : "21.11",
    //            "displayName" : "Release (21.11)",
    //            "tag" : "release"
    //        },
    //        "21.08"
    //    ]
    // }
    //
    // Documentation for a given version is expected to live in a directory
    // with the same name as the "tag" property, e.g. the docs for "21.11" live
    // in the "release" directory and the docs for "21.08" live in the "21.08"
    // directory.
    //
    // The switching functionality recognizes two special tags: "dev" and "release".
    //
    // This file is outside the documentation build; it is expected to live on
    // whatever server is hosting the docs publicly.
    $.getJSON("../versions.json", function(data) {

      var dev_version, release_version
      var all_versions = []
      $.each(data.versions, function(index, entry) {
        var version = create_version_object(entry);
        if (version.tag == 'release') {
            release_version = version;
        }
        else if (version.tag == 'dev') {
            dev_version = version;
        }

        all_versions.push(version);
      });

      var version_select = build_version_select(DOCUMENTATION_OPTIONS.VERSION, all_versions);
      $('.version_switcher_placeholder').html(version_select);
      $('.version_switcher_placeholder select').bind('change', on_version_switch);

      if (DOCUMENTATION_OPTIONS.VERSION != release_version.version) {
        var warningMsg;
        if (DOCUMENTATION_OPTIONS.VERSION == dev_version.version) {
          warningMsg = "This document is for a version of USD that is under development."
        }
        else {
          warningMsg = "This document is for an older version of USD."
        }

        warningMsg += " The current release is <a class=\"new_ver\">" 
              + release_version.version + "</a>";

        $('.version_warning_placeholder').html(warningMsg)          
        $('.version_warning_placeholder').addClass('version_warning')
        $('.new_ver').click(function() { 
            switch_to_tag('release');
        });
      }
    })
    .fail(function() {
      // We may fail to retrieve the versions.json for a number of reasons.
      // It may not exist, or we may have run into CORS restrictions (which
      // typically happens when viewing the documentation locally).
      // In this case, we just populate the selector with the current 
      // version so we can still see the dropdown.
      var all_versions = [create_version_object(DOCUMENTATION_OPTIONS.VERSION)];

      var version_select = build_version_select(DOCUMENTATION_OPTIONS.VERSION, all_versions);
      $('.version_switcher_placeholder').html(version_select);
      $('.version_switcher_placeholder select').bind('change', on_version_switch);
    });

  });

})();
