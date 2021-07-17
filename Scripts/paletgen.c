/*
  paletgen.c: Generate palettes from HTML gradients
  
  Copyright (C) 2020 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
  unsigned int i;
  unsigned int stops;
  float step;
  unsigned int r, g, b;
  const char *html;
  
  if (argc < 4) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s name #stop1 #stop2 [#stop3 [...]]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  stops = argc - 2;
  step = 255. / (stops - 1);


  printf("  <suscan:object class=\"palette\">\n");
  printf("    <suscan:field name=\"name\"><![CDATA[%s]]></suscan:field>\n", argv[1]);
  printf("    <suscan:object_set name=\"stops\">\n");

  for (i = 0; i < stops; ++i) {
    if (sscanf(argv[i + 2], "#%02x%02x%02x", &r, &g, &b) != 3) {
      fprintf(stderr, "%s: `%s' is not a valid HTML color.\n", argv[0], argv[i + 2]);
      exit(EXIT_FAILURE);
    }

    printf("      <suscan:object>\n");
    printf(
           "        <suscan:field name=\"position\" value=\"%d\" />\n",
      (int) (i * step));
    printf(
      "        <suscan:field name=\"red\" value=\"%g\" />\n",
      r / 255.);
    printf(
      "        <suscan:field name=\"green\" value=\"%g\" />\n",
      g / 255.);
    printf(
      "        <suscan:field name=\"blue\" value=\"%g\" />\n",
      b / 255.);
    printf("      </suscan:object>\n");
  }

  printf("    </suscan:object_set>\n");
  printf("  </suscan:object>\n");
  
  return 0;
}
  
