#include <dmzTypesSphere.h>
#include <dmzTypesVector.h>

#include <math.h>

/*!

\class dmz::Sphere
\ingroup Types
\brief Provides a sphere implementation of the Volume base class.
\details The origin of the sphere is its center. The radius must be greater than zero.

*/

struct dmz::Sphere::State {

   Vector origin;
   Float64 radius;
   Float64 radiusSquared;

   State &operator= (const State &Value) {

      origin = Value.origin;
      radius = Value.radius;
      radiusSquared = Value.radiusSquared;

      return *this;
   }

   Boolean operator== (const State &Value) const {

      return (origin == Value.origin) && is_zero64 (radius - Value.radius);
   }

   State () : radius (1.0), radiusSquared (1.0) {;}
   State (const State &Value) : radius (1.0), radiusSquared (1.0) { *this = Value; }
};

//! Base constructor.
dmz::Sphere::Sphere () : _state (*(new State)) {;}


//! Copy constructor.
dmz::Sphere::Sphere (const Sphere &Value) : _state (*(new State (Value._state))) {;}


/*!

\brief Constructor.
\param[in] Origin Vector containing the origin of the Sphere.
\param[in] Radius Scalar value containing the radius of the Sphere.

*/
dmz::Sphere::Sphere (const Vector &Origin, const Float64 Radius) :
      _state (*(new State)) {

   _state.origin = Origin;
   _state.radius = (Radius > 0.0 ? Radius : 1.0);
   _state.radiusSquared = (Radius > 0.0 ? Radius * Radius : 1.0);
}


//! Destructor.
dmz::Sphere::~Sphere () { delete &_state; }

void
dmz::Sphere::set_origin (const Vector &Origin) { _state.origin = Origin; }


dmz::Vector
dmz::Sphere::get_origin () const { return _state.origin; }


dmz::Boolean
dmz::Sphere::contains_point (const Vector &Point) const {

   return (Point - _state.origin).magnitude_squared () <= _state.radiusSquared;
}


void
dmz::Sphere::get_extents (Vector &origin, Vector &min, Vector &max) const {

   origin = _state.origin;
   min = _state.origin - _state.radius;
   max = _state.origin + _state.radius;
}


//! Assignment operator.
dmz::Sphere &
dmz::Sphere::operator= (const Sphere &Value) { _state = Value._state; return *this; }


//! Relational "equal to" operator.
dmz::Boolean
dmz::Sphere::operator== (const Sphere &Value) const { return _state == Value._state; }


//! Sets the radius of the Sphere.
void
dmz::Sphere::set_radius (const Float64 Radius) {

   if (Radius > 0.0) {

      _state.radius = Radius;
      _state.radiusSquared = Radius * Radius;
   }
}


//! Gets the radius of the Sphere.
dmz::Float64
dmz::Sphere::get_radius () { return _state.radius; }

