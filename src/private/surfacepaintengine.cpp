
#include <float.h>

#include "./misc.h"
#include "./painter.h"
#include "./surfacepainter.h"
#include "surfacepaintengine.h"

namespace Malachite
{

SurfacePaintEngine::SurfacePaintEngine() :
	PaintEngine()
{}

SurfacePaintEngine::~SurfacePaintEngine()
{}

bool SurfacePaintEngine::begin(Paintable *paintable)
{
	Surface *surface = dynamic_cast<Surface *>(paintable);
	if (!surface)
		return false;
	
	_surface = surface;
	return true;
}

bool SurfacePaintEngine::flush()
{
	return true;
}

void SurfacePaintEngine::drawPreTransformedPolygons(const FixedMultiPolygon &polygons)
{
	QRect boundingRect = polygons.boundingRect().toAlignedRect();
	
	QPointSet keys = Surface::rectToKeys(boundingRect);
	if (!_keyClip.isEmpty())
		keys &= _keyClip;
	
	for (const QPoint &key : keys)
	{
		FixedMultiPolygon rectShape = FixedPolygon::fromRect(Surface::keyToRect(key));
		FixedMultiPolygon clippedShape = rectShape & polygons;
		
		QPoint delta = -key * Surface::tileWidth();
		
		clippedShape.translate(delta);
		
		Painter painter(&_surface->tileRef(key));
		*painter.state() = *state();
		painter.setShapeTransform(state()->shapeTransform * QTransform::fromTranslate(delta.x(), delta.y()));
		
		painter.drawPreTransformedPolygons(clippedShape);
	}
}

void SurfacePaintEngine::drawPreTransformedImage(const QPoint &point, const Image &image)
{
	QPointSet keys = Surface::rectToKeys(QRect(point, image.size()));
	if (!_keyClip.isEmpty())
		keys &= _keyClip;
	
	for (const QPoint &key : keys)
	{
		Painter painter(&_surface->tileRef(key));
		*painter.state() = *state();
		painter.drawPreTransformedImage(point - key * Surface::tileWidth(), image);
	}
}

void SurfacePaintEngine::drawPreTransformedImage(const QPoint &point, const Image &image, const QRect &imageMaskRect)
{
	QPointSet keys = Surface::rectToKeys((imageMaskRect & image.rect()).translated(point));
	if (!_keyClip.isEmpty())
		keys &= _keyClip;
	
	for (const QPoint &key : keys)
	{
		Painter painter(&_surface->tileRef(key));
		*painter.state() = *state();
		
		QPoint delta = key * Surface::tileWidth();
		
		painter.drawPreTransformedImage(point - delta, image, imageMaskRect.translated(-delta));
	}
}

void SurfacePaintEngine::drawPreTransformedSurface(const QPoint &point, const Surface &surface)
{
	if (point == QPoint())
	{
		QPointSet keys = surface.keys() | _surface->keys();
		
		if (!_keyRectClip.isEmpty())
			keys &= _keyRectClip.keys().toSet();
		else
		{
			if (!_keyClip.isEmpty())
				keys &= _keyClip;
		}
		
		for (const QPoint &key : keys)
		{
			BlendOp::TileCombination combination = BlendOp::NoTile;
			
			if (_surface->contains(key))
				combination |= BlendOp::TileDestination;
			if (surface.contains(key))
				combination |= BlendOp::TileSource;
			
			switch (BlendMode(state()->blendMode).op()->tileRequirement(combination))
			{
				case BlendOp::TileSource:
					_surface->tileRef(key) = surface.tile(key) * state()->opacity;
					break;
					
				case BlendOp::NoTile:
					_surface->remove(key);
					break;
					
				default:
				case BlendOp::TileDestination:
					break;
					
				case BlendOp::TileBoth:
					
					Painter painter(&_surface->tileRef(key));
					painter.setBlendMode(state()->blendMode);
					painter.setOpacity(state()->opacity);
					
					if (!_keyRectClip.isEmpty())
						painter.drawPreTransformedImage(QPoint(), surface.tile(key), _keyRectClip[key]);
					else
						painter.drawPreTransformedImage(QPoint(), surface.tile(key));
					break;
			}
		}
	}
	else
	{
		for (const QPoint &key : surface.keys())
			drawPreTransformedImage(point + key * Surface::tileWidth(), surface.tile(key));
	}
}

}

