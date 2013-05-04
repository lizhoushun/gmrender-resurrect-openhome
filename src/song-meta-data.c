/* song-meta-data - Object holding meta data for a song.
 *
 * Copyright (C) 2012 Henner Zellre
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */ 

#include "song-meta-data.h"

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xmlescape.h"

void SongMetaData_init(struct SongMetaData *value) {
	memset(value, 0, sizeof(struct SongMetaData));
}
void SongMetaData_clear(struct SongMetaData *value) {
	free(value->title);
	value->title = NULL;
	free(value->artist);
	value->artist = NULL;
	free(value->album);
	value->album = NULL;
	free(value->genre);
	value->genre = NULL;
}

static const char kDidlHeader[] = "<DIDL-Lite "
	"xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" "
	"xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
	"xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">";
static const char kDidlFooter[] = "</DIDL-Lite>";

static char *generate_DIDL(const char *id,
			   const char *title, const char *artist,
			   const char *album, const char *genre,
			   const char *composer) {
	char *result = NULL;
	asprintf(&result, "%s\n<item id=\"%s\">\n"
		 "\t<dc:title>%s</dc:title>\n"
		 "\t<upnp:artist>%s</upnp:artist>\n"
		 "\t<upnp:album>%s</upnp:album>\n"
		 "\t<upnp:genre>%s</upnp:genre>\n"
		 "\t<upnp:creator>%s</upnp:creator>\n"
		 "</item>\n%s",
		 kDidlHeader, id,
		 title ? title : "", artist ? artist : "",
		 album ? album : "", genre ? genre : "",
		 composer ? composer : "",
		 kDidlFooter);
	return result;
}

// Takes input, if it finds the given tag, then replaces the content between
// these with 'content'. It might re-allocate the original string; only the
// returned string is valid.
// Very crude way to edit XML.
static char *replace_range(char *input,
			   const char *tag_start, const char *tag_end,
			   const char *content) {
	if (content == NULL)  // unknown content; document unchanged.
		return input;
	//fprintf(stderr, "------- Before ------\n%s\n-------------\n", input);
	const int total_len = strlen(input);
	const char *start_pos = strstr(input, tag_start);
	if (start_pos == NULL) return input;
	start_pos += strlen(tag_start);
	const char *end_pos = strstr(start_pos, tag_end);
	if (end_pos == NULL) return input;
	int old_content_len = end_pos - start_pos;
	int new_content_len = strlen(content);
	char *result = NULL;
	if (old_content_len != new_content_len) {
		result = (char*)malloc(total_len
				       + new_content_len - old_content_len + 1);
		strncpy(result, input, start_pos - input);
		strncpy(result + (start_pos - input), content, new_content_len);
		strcpy(result + (start_pos - input) + new_content_len, end_pos);
		free(input);
	} else {
		// Typically, we replace the same content with itself - same
		// length. No realloc in this case.
		strncpy(input + (start_pos - input), content, new_content_len);
		result = input;
	}
	//fprintf(stderr, "------- After ------\n%s\n-------------\n", result);
	return result;
}

// TODO: actually use some XML library for this, but spending too much time
// with XML is not good for the brain :) Worst thing that came out of the 90ies.
char *SongMetaData_to_DIDL(const struct SongMetaData *object,
			   const char *original_xml) {
	// Generating a unique ID in case the players cache the content by
	// the item-ID. Right now this is experimental and not known to make
	// any difference - it seems that players just don't display changes
	// in the input stream. Grmbl.
	static unsigned int xml_id = 42;
	char unique_id[4 + 8 + 1];
	snprintf(unique_id, sizeof(unique_id), "gmr-%08x", xml_id++);

	char *result;
	char *title, *artist, *album, *genre, *composer;
	title = object->title ? xmlescape(object->title, 0) : NULL;
	artist = object->artist ? xmlescape(object->artist, 0) : NULL;
	album = object->album ? xmlescape(object->album, 0) : NULL;
	genre = object->genre ? xmlescape(object->genre, 0) : NULL;
	composer = object->composer ? xmlescape(object->composer, 0) : NULL;
	if (original_xml == NULL || strlen(original_xml) == 0) {
		result = generate_DIDL(unique_id, title, artist, album,
				       genre, composer);
	} else {
		// Otherwise, surgically edit the original document to give
		// control points as close as possible what they sent themself.
		result = strdup(original_xml);
		result = replace_range(result, "<dc:title>", "</dc:title>",
				       title);
		result = replace_range(result,
				       "<upnp:artist>", "</upnp:artist>",
				       artist);
		result = replace_range(result, "<upnp:album>", "</upnp:album>",
				       album);
		result = replace_range(result, "<upnp:genre>", "</upnp:genre>",
				       genre);
		result = replace_range(result,
				       "<upnp:creator>", "</upnp:creator>",
				       composer);
		result = replace_range(result, "id=\"", "\"", unique_id);
	}
	free(title);
	free(artist);
	free(album);
	free(genre);
	free(composer);
	return result;
}
