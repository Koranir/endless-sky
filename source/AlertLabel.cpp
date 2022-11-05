/* AlertLabel.cpp
Copyright (c) 2022 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "AlertLabel.h"

#include "Angle.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "Government.h"
#include "LineShader.h"
#include "Projectile.h"
#include "pi.h"
#include "PointerShader.h"
#include "RingShader.h"
#include "Ship.h"
#include "Sprite.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	const double GAP = 3.;
}



AlertLabel::AlertLabel(Point &position, const Projectile &projectile, shared_ptr<Ship> flagship, double zoom) : position(position * zoom), zoom(zoom)
{

	isTargetingMe = projectile.TargetPtr() == flagship;
	isDangerous = projectile.GetWeapon().HullDamage() > flagship->Attributes().Get("hull")/10;
	damagePercent = projectile.GetWeapon().HullDamage() / flagship->Attributes().Get("Hull");

	if(!isTargetingMe)
		color = Color(.8f, .8f, .4f, .5f);
	else
		color = Color(5.f, .48f, .16f, .5f);

	if(isDangerous)
		color = Color(1.f, .6f, .4f, .5f);

	radius = zoom*projectile.Radius()*0.75;
	rotation = projectile.Facing().Degrees() + 180.;
}



void AlertLabel::Draw() const
{
	double angle[3] = {150., 30., 270.};
	float dangerScale = 1.5 * min(damagePercent, 1.f);
	for(int i = 0; i < 3; i++)
	{
		RingShader::Draw(position, radius, 1.2f, .16f, color, 0.f, angle[i] + rotation);
		if(isTargetingMe)
			PointerShader::Draw(position, Angle(angle[i] + 30. + rotation).Unit(), 7.5f, (i ? 10.f : 22.f)*zoom, radius + (i ? 10 : 20)*zoom, color);
	}
	if(isDangerous)
		RingShader::Draw(position, radius + GAP + dangerScale, dangerScale, 1.f, Color(1.f, 0.1f, 0.1f, .7f));
}
