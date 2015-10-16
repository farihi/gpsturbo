# **gpsturbo** is an application for geocaching. #

![http://gpsturbo.googlecode.com/svn/gpsturbo.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo.jpg)

It allows viewing of geocaches on maps and printing them. It also allows you to create custom filters and upload and download geocaches to your GPSr. You can download tracks from your GPSr and view them overlayed on maps. It currently supports Garmin's IMG format maps, Openstreetmap format (xml) maps as well as downloadable tile based maps.

For uploading and downloading to your GPSr it uses GPSBabel

It currently runs on Windows, Linux and Macintosh. I would apreciate help getting it onto more systems as well.

It is written in c++ and uses the **kGUI** library which is here: http://code.google.com/p/kgui/

### If you would like to join this project, report a bug or make a comment please so do on my blog page ###

## Current Features Include ##
  * Loading of GPX files
  * Viewing Geocaches in a spreadsheet like table, with full table customization, column hide/show, order rearrange, change font size
  * Viewing of Geocaches on the current map
  * Adjustable bar between the Geocache spreadsheet and the Map to make either larger or smaller
  * Custom filters to view subsets of the Geocache list in the table
  * Filters can reference polygonal map areas and determine if the cache is inside or outside a given polygon, useful for areas around islands or lakes that are not easily accessable
  * Geocache labels on the map can be drawn in different colors using a filter
  * Map formats include Garmin IMG format maps, downloadable ( and Cached ) tile based maps (Google), plus Openstreetmap format maps (xml format converted to custom binary format)
  * Map overlays (with selectable opacity) can be shown over the current base map, overlay formats currently supported are KML and KMZ
  * Geocaches can be uploaded and downloaded to your GPSr ( it uses GPSBabel internally )
  * You can download and upload tracks from your GPSr and view them overlayed ( in different colors ) on the current map.
  * You can write macros for repetitive tasks ( Basic )
  * Custom routes can be generated.
  * It can print single page maps as well as large maps up to 10 pages wide by 10 pages tall. It also can optionally have Cache information on the right edge of the page for things like hints etc.
  * It handles realtime positioning of the map when connected to a GPSr that supports it.
  * It can download your updated "found" list from your GPSr and list all the newly found Caches, then you can click on them one at a time and log them on Geocaching.com
  * It has a solver to do projections, calc distances between waypoints, calc sunrise / sunset, do simple decoding or encoding of encrypted text
  * It has a lines mode where you can have lines, arcs and circles drawn on the map, this can be useful for generating Geocache puzzles or solving the puzzles of others.
  * You can view the webpage for a Cache in the built-in browser or launch and external browser like Firefox or IE
  * You can capture a selected area of the current Map and save it as a JPG image. large files >50MB are possible
  * You can load a GPX file's 'Found' flags into one of the 8 usertick flags and use these ( with an appropriate filter ) to plan caching trips with multiple people
  * You can export a section of the currently displayed map as a Jpeg or PNG image, or in KML or KMZ format.


## Planned Features Include ##
  * Multi Language support
  * Road routing between caches for generating a "Cache Machine"
  * Figure out projection for OZI Explorer maps ( rendering works now )
  * Search 'text' inside Garmin IMG format maps
  * Allow 'basic' code to be called in a filter.
  * Add other boolean operators ( or / xor and brackets ) to be used in filters
  * Add functionality to the Basic macro interface as requested
  * Figure out the new compressed IMG format for Garmin Maps

You can see the new and improved Garmin IMG map rendering here:
![http://gpsturbo.googlecode.com/svn/gpsturbo11.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo11.jpg)

Plus more screenshots of it here:

![http://gpsturbo.googlecode.com/svn/gpsturbo2.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo2.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo3.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo3.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo4.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo4.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo5.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo5.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo6.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo6.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo7.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo7.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo8.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo8.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo9.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo9.jpg)
![http://gpsturbo.googlecode.com/svn/gpsturbo10.jpg](http://gpsturbo.googlecode.com/svn/gpsturbo10.jpg)

http://www.scale18.com/cgi-bin/page/gpsturbo.html