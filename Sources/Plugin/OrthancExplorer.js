/**
 * Neuroimaging plugin for Orthanc
 * Copyright (C) 2021-2022 Sebastien Jodogne, UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


$('#series').live('pagebeforecreate', function() {
  var b = $('<a>')
      .attr('data-role', 'button')
      .attr('href', '#')
      .attr('data-icon', 'search')
      .attr('data-theme', 'e')
      .text('Export to NIfTI');

  b.insertBefore($('#series-delete').parent().parent());
  b.click(function(e) {
    if ($.mobile.pageData) {
      e.preventDefault();  // stop the browser from following
      window.location.href = '../series/' + $.mobile.pageData.uuid + '/nifti';
    }
  });
});


$('#instance').live('pagebeforecreate', function() {
  var b = $('<a>')
      .attr('data-role', 'button')
      .attr('href', '#')
      .attr('data-icon', 'search')
      .attr('data-theme', 'e')
      .text('Export to NIfTI');

  b.insertBefore($('#instance-delete').parent().parent());
  b.click(function(e) {
    if ($.mobile.pageData) {
      e.preventDefault();  // stop the browser from following
      window.location.href = '../instances/' + $.mobile.pageData.uuid + '/nifti';
    }
  });
});
