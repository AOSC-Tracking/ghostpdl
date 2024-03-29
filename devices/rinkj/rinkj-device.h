/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/

#ifndef rinkj_device_h_INCLUDED
#define rinkj_device_h_INCLUDED

/* The device abstraction within the Rinkj driver. */

typedef struct _RinkjDevice RinkjDevice;
typedef struct _RinkjDeviceParams RinkjDeviceParams;

struct _RinkjDeviceParams {
  int width;
  int height;
  int n_planes;
  const char *plane_names;
};

struct _RinkjDevice {
  int (*set) (RinkjDevice *self, const char *config);
  int (*init) (RinkjDevice *self, const RinkjDeviceParams *params);
  int (*write) (RinkjDevice *self, const char **data);
  int init_happened;
};

/* Deprecated */
int
rinkj_device_set (RinkjDevice *self, const char *config);

int
rinkj_device_set_param (RinkjDevice *self, const char *key,
                        const char *value, int value_size);

/* Convenience functions */
int
rinkj_device_set_param_string (RinkjDevice *self, const char *key,
                               const char *value);
int
rinkj_device_set_param_int (RinkjDevice *self, const char *key, int value);

int
rinkj_device_init (RinkjDevice *self, const RinkjDeviceParams *params);

int
rinkj_device_write (RinkjDevice *self, const char **data);

#endif
