#include "distribution.h"

#include <random>

#include "json.h"
#include "rng.h"

struct int_distribution_impl {
    virtual ~int_distribution_impl() = default;
    virtual int minimum() const = 0;
    virtual int sample() = 0;
    virtual std::string description() const = 0;
};

struct fixed_distribution : int_distribution_impl {
    int value;

    explicit fixed_distribution( int v )
        : value( v )
    {}

    int minimum() const override {
        return value;
    }

    int sample() override {
        return value;
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Fixed(%d)", value );
    }
};

struct uniform_distribution : int_distribution_impl {
    std::uniform_int_distribution<int> dist;

    explicit uniform_distribution( int a, int b )
        : dist( a, b )
    {}

    int minimum() const override {
        return 0;
    }

    int sample() override {
        return dist( rng_get_engine() );
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Uniform(%d,%d)", dist.a(), dist.b());
    }
};

struct binomial_distribution : int_distribution_impl {
    std::binomial_distribution<int> dist;

    explicit binomial_distribution( int t, double p )
        : dist( t, p )
    {}

    int minimum() const override {
        return 0;
    }

    int sample() override {
        return dist( rng_get_engine() );
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Binomial(%d,%.0f)", dist.t(), dist.p());
    }
};

struct poisson_distribution : int_distribution_impl {
    std::poisson_distribution<int> dist;

    explicit poisson_distribution( double mean )
        : dist( mean )
    {}

    int minimum() const override {
        return 0;
    }

    int sample() override {
        return dist( rng_get_engine() );
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Poisson(%.0f)", dist.mean() );
    }
};

int_distribution::int_distribution()
    : impl_( make_shared_fast<fixed_distribution>( 0 ) )
{}

int_distribution::int_distribution( int v )
    : impl_( make_shared_fast<fixed_distribution>( v ) )
{}

int int_distribution::minimum() const
{
    return impl_->minimum();
}

int int_distribution::sample() const
{
    return impl_->sample();
}

std::string int_distribution::description() const
{
    return impl_->description();
}

void int_distribution::deserialize( const JsonValue &jin )
{
	int lo = 0;
	int hi = INT_MAX;
    if( jin.test_int() ) {
        int v = jin.get_int();
        impl_ = make_shared_fast<fixed_distribution>( v );
    } else if( jin.test_object() ) {
        JsonObject jo = jin.get_object();
        if( jo.has_member( "bounds" ) ) {
			if( jo.get_array( "bounds" ).size() != 2 ) {
				jo.throw_error( "bounds array has wrong number of elements, should be [ lo, hi ]" );
			}
			lo = jo.get_array( "bounds" ).get_int( 0 );
			hi = jo.get_array( "bounds" ).get_int( 1 );
			if( lo >= hi ) {
				jo.throw_error( "bounds array should be in order [ lo, hi ]" );
			}
		}
		if( jo.has_member( "distribution" ) ) {
			if( jo.get_object( "distribution" ).has_member( "poisson" ) ) {
				double mean = jo.get_float( "poisson", 1.0 );
				if( mean <= 0.0 ) {
					jo.throw_error( "poisson mean must be greater than 0.0" );
				}
				impl_ = make_shared_fast<poisson_distribution>( mean );
				if( impl_ < lo ) {
					impl_ = lo;
				} else if( impl_ > hi ) {
					impl_ = hi;
				}
			} else if( jo.get_object( "distribution" ).has_member( "binomial" ) ) {
				JsonArray arr = jo.get_array( "binomial" );
				if( jo.get_array( "binomial" ).size() != 2 ) {
					jo.throw_error( "binomial array has wrong number of elements, should be [ t, p ]" );
				}
				int t = jo.get_array( "binomial" ).get_int( 0 );
				double p = jo.get_array( "binomial" ).get_float( 1 );
				if( t < 0 ) {
					jo.throw_error( "t trials must be 0 or greater" );
				}
				if( p < 0.0 || p > 1.0 ) {
					jo.throw_error( "success probability must be between 0.0 and 1.0" );
				}
				impl_ =  make_shared_fast<binomial_distribution>( t, p );
				if( impl_ < lo ) {
					impl_ = lo;
				} else if( binomial > hi ) {
					impl_ = hi;
				}
			} else {
			jo.throw_error( R"(Expected "poisson" or "binomial" member)" );
			}
		} else if( jo.has_member( "bounds" ) ) {
			impl_ = make_shared_fast<uniform_distribution>( lo, hi );
		} else {
			jo.throw_error( R"(Expected "bounds" and/or "distribution" member)" );
		}
    } else {
        jin.throw_error( "expected integer or object" );
    }
}
