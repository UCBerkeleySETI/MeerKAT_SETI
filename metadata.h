#include <stdint.h>
#include <stdio.h>
#include "hashpipe.h"
#include "hashpipe_databuf.h"


struct beamCoord {
  float ra[NBEAMS];
  float dec[NBEAMS];
};

struct antCoord {
  float east[NANTS];
  float north[NANTS];
  float up[NANTS];
};

