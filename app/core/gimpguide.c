/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpguide.h"
#include "gimpmarshal.h"


enum
{
  REMOVED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_STYLE
};


struct _GimpGuidePrivate
{
  guint32              guide_ID;
  GimpOrientationType  orientation;
  gint                 position;

  GimpGuideStyle       style;
};


static void   gimp_guide_get_property (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);
static void   gimp_guide_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);


G_DEFINE_TYPE (GimpGuide, gimp_guide, G_TYPE_OBJECT)

static guint gimp_guide_signals[LAST_SIGNAL] = { 0 };


static void
gimp_guide_class_init (GimpGuideClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_guide_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpGuideClass, removed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);


  object_class->get_property = gimp_guide_get_property;
  object_class->set_property = gimp_guide_set_property;

  klass->removed             = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_uint ("id", NULL, NULL,
                                                      0, G_MAXUINT32, 0,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      GIMP_PARAM_READWRITE));

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         NULL, NULL,
                         GIMP_TYPE_ORIENTATION_TYPE,
                         GIMP_ORIENTATION_UNKNOWN,
                         0);

  GIMP_CONFIG_PROP_INT (object_class, PROP_POSITION,
                        "position",
                        NULL, NULL,
                        GIMP_GUIDE_POSITION_UNDEFINED,
                        GIMP_MAX_IMAGE_SIZE,
                        GIMP_GUIDE_POSITION_UNDEFINED,
                        0);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         NULL, NULL,
                         GIMP_TYPE_GUIDE_STYLE,
                         GIMP_GUIDE_STYLE_NONE,
                         0);

  g_type_class_add_private (klass, sizeof (GimpGuidePrivate));
}

static void
gimp_guide_init (GimpGuide *guide)
{
  guide->priv = G_TYPE_INSTANCE_GET_PRIVATE (guide, GIMP_TYPE_GUIDE,
                                             GimpGuidePrivate);
}

static void
gimp_guide_get_property (GObject      *object,
                         guint         property_id,
                         GValue       *value,
                         GParamSpec   *pspec)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, guide->priv->guide_ID);
      break;
    case PROP_ORIENTATION:
      g_value_set_enum (value, guide->priv->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, guide->priv->position);
      break;
    case PROP_STYLE:
      g_value_set_enum (value, guide->priv->style);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_guide_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  switch (property_id)
    {
    case PROP_ID:
      guide->priv->guide_ID = g_value_get_uint (value);
      break;
    case PROP_ORIENTATION:
      guide->priv->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      guide->priv->position = g_value_get_int (value);
      break;
    case PROP_STYLE:
      guide->priv->style = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpGuide *
gimp_guide_new (GimpOrientationType  orientation,
                guint32              guide_ID)
{
  return g_object_new (GIMP_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       "style",       GIMP_GUIDE_STYLE_NORMAL,
                       NULL);
}

/**
 * gimp_guide_custom_new:
 * @orientation:       the #GimpOrientationType
 * @guide_ID:          the unique guide ID
 * @guide_style:       the #GimpGuideStyle
 *
 * This function returns a new guide and will flag it as "custom".
 * Custom guides are used for purpose "other" than the basic guides
 * a user can create oneself, for instance as symmetry guides, to
 * drive GEGL ops, etc.
 * They are not saved in the XCF file. If an op, a symmetry or a plugin
 * wishes to save its state, it has to do it internally.
 * Moreover they don't follow guide snapping settings and never snap.
 *
 * Returns: the custom #GimpGuide.
 **/
GimpGuide *
gimp_guide_custom_new (GimpOrientationType  orientation,
                       guint32              guide_ID,
                       GimpGuideStyle       guide_style)
{
  GimpGuide *guide;

  guide = g_object_new (GIMP_TYPE_GUIDE,
                        "id",                guide_ID,
                        "orientation",       orientation,
                        "style",             guide_style,
                        NULL);

  return guide;
}

guint32
gimp_guide_get_ID (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), 0);

  return guide->priv->guide_ID;
}

GimpOrientationType
gimp_guide_get_orientation (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), GIMP_ORIENTATION_UNKNOWN);

  return guide->priv->orientation;
}

void
gimp_guide_set_orientation (GimpGuide           *guide,
                            GimpOrientationType  orientation)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guide->priv->orientation = orientation;

  g_object_notify (G_OBJECT (guide), "orientation");
}

gint
gimp_guide_get_position (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), GIMP_GUIDE_POSITION_UNDEFINED);

  return guide->priv->position;
}

void
gimp_guide_set_position (GimpGuide *guide,
                         gint       position)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  guide->priv->position = position;

  g_object_notify (G_OBJECT (guide), "position");
}

void
gimp_guide_removed (GimpGuide *guide)
{
  g_return_if_fail (GIMP_IS_GUIDE (guide));

  g_signal_emit (guide, gimp_guide_signals[REMOVED], 0);
}

GimpGuideStyle
gimp_guide_get_style (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), GIMP_GUIDE_STYLE_NONE);

  return guide->priv->style;
}

gboolean
gimp_guide_is_custom (GimpGuide *guide)
{
  g_return_val_if_fail (GIMP_IS_GUIDE (guide), FALSE);

  return guide->priv->style != GIMP_GUIDE_STYLE_NORMAL;
}
