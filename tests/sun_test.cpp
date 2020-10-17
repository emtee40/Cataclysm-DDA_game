#include "cata_catch.h"
#include "calendar.h" // IWYU pragma: associated

#include <iomanip>
#include <string>
#include <unordered_set>

#include "hash_utils.h"
#include "line.h"
#include "optional.h"
#include "output.h"
#include "stringmaker.h"
#include "units_utility.h"

// SUN TESTS

// The 24-hour solar cycle is divided into four parts, as returned by four calendar.cpp functions:
//
// is_night : from the end of dusk, until the following sunrise (start of dawn)
// is_dawn  : begins at sunrise, lasts twilight_duration (1 hour)
// is_day   : from the end of dawn, until sunset (start of dusk)
// is_dusk  : begins at sunset, lasts twilight_duration (1 hour)
//
// These are inclusive at their endpoints; in other words, each overlaps with the next like so:
//
// 00:00 is_night
//   :   is_night
// 06:00 is_night && is_dawn ( sunrise )
//   :   is_dawn ( sunrise + twilight )
// 07:00 is_dawn && is_day
//   :   is_day
// 19:00 is_day && is_dusk ( sunset )
//   :   is_dusk ( sunset + twilight )
// 20:00 is_dusk && is_night
//   :   is_night
// 23:59 is_night
//
// The times of sunrise and sunset will naturally depend on the current time of year; this aspect is
// covered by the "sunrise and sunset" and solstice/equinox tests later in this file. Here we simply
// use the first day of spring as a baseline.
//
// This test covers is_night, is_dawn, is_day, is_dusk, and their behavior in relation to time of day.
TEST_CASE( "daily solar cycle", "[sun][night][dawn][day][dusk]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point noon = calendar::turn_zero + 12_hours;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    REQUIRE( "Year 1, Spring, day 1 6:00:00 AM" == to_string( today_sunrise ) );
    REQUIRE( "Year 1, Spring, day 1 7:00:00 PM" == to_string( today_sunset ) );

    // First, at the risk of stating the obvious
    CHECK( is_night( midnight ) );
    // And while we're at it
    CHECK_FALSE( is_day( midnight ) );
    CHECK_FALSE( is_dawn( midnight ) );
    CHECK_FALSE( is_dusk( midnight ) );

    // Yep, still dark
    CHECK( is_night( midnight + 1_seconds ) );
    CHECK( is_night( midnight + 2_hours ) );
    CHECK( is_night( midnight + 3_hours ) );
    CHECK( is_night( midnight + 4_hours ) );

    // The point of sunrise is both "night" and "dawn"
    CHECK( is_night( today_sunrise ) );
    CHECK( is_dawn( today_sunrise ) );

    // Dawn
    CHECK_FALSE( is_night( today_sunrise + 1_seconds ) );
    CHECK( is_dawn( today_sunrise + 1_seconds ) );
    CHECK( is_dawn( today_sunrise + 30_minutes ) );
    CHECK( is_dawn( today_sunrise + 1_hours - 1_seconds ) );
    CHECK_FALSE( is_day( today_sunrise + 1_hours - 1_seconds ) );

    // The endpoint of dawn is both "dawn" and "day"
    CHECK( is_dawn( today_sunrise + 1_hours ) );
    CHECK( is_day( today_sunrise + 1_hours ) );

    // Breakfast
    CHECK_FALSE( is_dawn( today_sunrise + 1_hours + 1_seconds ) );
    CHECK( is_day( today_sunrise + 1_hours + 1_seconds ) );
    CHECK( is_day( today_sunrise + 2_hours ) );
    // Second breakfast
    CHECK( is_day( today_sunrise + 3_hours ) );
    CHECK( is_day( today_sunrise + 4_hours ) );
    // Luncheon
    CHECK( is_day( noon - 3_hours ) );
    CHECK( is_day( noon - 2_hours ) );
    // Elevenses
    CHECK( is_day( noon - 1_hours ) );

    // Noon
    CHECK( is_day( noon ) );
    CHECK_FALSE( is_dawn( noon ) );
    CHECK_FALSE( is_dusk( noon ) );
    CHECK_FALSE( is_night( noon ) );

    // Afternoon tea
    CHECK( is_day( noon + 1_hours ) );
    CHECK( is_day( noon + 2_hours ) );
    // Dinner
    CHECK( is_day( noon + 3_hours ) );
    CHECK( is_day( today_sunset - 2_hours ) );
    // Supper
    CHECK( is_day( today_sunset - 1_hours ) );
    CHECK( is_day( today_sunset - 1_seconds ) );
    CHECK_FALSE( is_dusk( today_sunset - 1_seconds ) );

    // The beginning of sunset is both "day" and "dusk"
    CHECK( is_day( today_sunset ) );
    CHECK( is_dusk( today_sunset ) );

    // Dusk
    CHECK_FALSE( is_day( today_sunset + 1_seconds ) );
    CHECK( is_dusk( today_sunset + 1_seconds ) );
    CHECK( is_dusk( today_sunset + 30_minutes ) );
    CHECK( is_dusk( today_sunset + 1_hours - 1_seconds ) );
    CHECK_FALSE( is_night( today_sunset + 1_hours - 1_seconds ) );

    // The point when dusk ends is both "dusk" and "night"
    CHECK( is_dusk( today_sunset + 1_hours ) );
    CHECK( is_night( today_sunset + 1_hours ) );

    // Night again
    CHECK( is_night( today_sunset + 1_hours + 1_seconds ) );
    CHECK( is_night( today_sunset + 2_hours ) );
    CHECK( is_night( today_sunset + 3_hours ) );
    CHECK( is_night( today_sunset + 4_hours ) );
}

// The calendar `sunlight` function returns light level for both sun and moon.
TEST_CASE( "sunlight and moonlight", "[sun][sunlight][moonlight]" )
{
    // Use sunrise/sunset on the first day (spring equinox)
    static const time_point midnight = calendar::turn_zero;
    static const time_point today_sunrise = sunrise( midnight );
    static const time_point today_sunset = sunset( midnight );

    // Expected numbers below assume 100.0f maximum daylight level
    // (maximum daylight is different at other times of year - see [daylight] tests)
    REQUIRE( 100.0f == default_daylight_level() );

    SECTION( "sunlight" ) {
        // Before dawn
        CHECK( 1.0f == sunlight( midnight ) );
        CHECK( 1.0f == sunlight( today_sunrise ) );
        // Dawn
        CHECK( 1.0275f == sunlight( today_sunrise + 1_seconds ) );
        CHECK( 2.65f == sunlight( today_sunrise + 1_minutes ) );
        CHECK( 25.75f == sunlight( today_sunrise + 15_minutes ) );
        CHECK( 50.50f == sunlight( today_sunrise + 30_minutes ) );
        CHECK( 75.25f == sunlight( today_sunrise + 45_minutes ) );
        // 1 second before full daylight
        CHECK( 99.9725f == sunlight( today_sunrise + 1_hours - 1_seconds ) );
        CHECK( 100.0f == sunlight( today_sunrise + 1_hours ) );
        // End of dawn, full light all day
        CHECK( 100.0f == sunlight( today_sunrise + 2_hours ) );
        CHECK( 100.0f == sunlight( today_sunrise + 3_hours ) );
        // Noon
        CHECK( 100.0f == sunlight( midnight + 12_hours ) );
        CHECK( 100.0f == sunlight( midnight + 13_hours ) );
        CHECK( 100.0f == sunlight( midnight + 14_hours ) );
        // Dusk begins
        CHECK( 100.0f == sunlight( today_sunset ) );
        // 1 second after dusk begins
        CHECK( 99.9725f == sunlight( today_sunset + 1_seconds ) );
        CHECK( 75.25f == sunlight( today_sunset + 15_minutes ) );
        CHECK( 50.50f == sunlight( today_sunset + 30_minutes ) );
        CHECK( 25.75f == sunlight( today_sunset + 45_minutes ) );
        // 1 second before full night
        CHECK( 1.0275f == sunlight( today_sunset + 1_hours - 1_seconds ) );
        CHECK( 1.0f == sunlight( today_sunset + 1_hours ) );
        // After dusk
        CHECK( 1.0f == sunlight( today_sunset + 2_hours ) );
        CHECK( 1.0f == sunlight( today_sunset + 3_hours ) );
    }

    // This moonlight test is intentionally simple, only checking new moon (minimal light) and full
    // moon (maximum moonlight). More detailed tests of moon phase and light should be expressed in
    // `moon_test.cpp`. Including here simply to check that `sunlight` also calculates moonlight.
    SECTION( "moonlight" ) {
        static const time_duration phase_time = calendar::season_length() / 6;
        static const time_point new_moon = calendar::turn_zero;
        static const time_point full_moon = new_moon + phase_time;

        WHEN( "the moon is new" ) {
            REQUIRE( get_moon_phase( new_moon ) == MOON_NEW );
            THEN( "moonlight is 1.0" ) {
                CHECK( 1.0f == sunlight( new_moon ) );
            }
        }

        WHEN( "the moon is full" ) {
            REQUIRE( get_moon_phase( full_moon ) == MOON_FULL );
            THEN( "moonlight is 10.0" ) {
                CHECK( 10.0f == sunlight( full_moon ) );
            }
        }
    }
}

// current_daylight_level returns seasonally-adjusted maximum daylight level
TEST_CASE( "current daylight level", "[sun][daylight][equinox][solstice]" )
{
    static const time_duration one_season = calendar::season_length();
    static const time_point spring = calendar::turn_zero;
    static const time_point summer = spring + one_season;
    static const time_point autumn = summer + one_season;
    static const time_point winter = autumn + one_season;

    SECTION( "baseline 100 daylight on the spring and autumn equinoxes" ) {
        CHECK( 100.0f == current_daylight_level( spring ) );
        CHECK( 100.0f == current_daylight_level( autumn ) );
    }

    SECTION( "25 percent more daylight on the summer solstice" ) {
        CHECK( 125.0f == current_daylight_level( summer ) );
    }

    SECTION( "25 percent less daylight on the winter solstice" ) {
        CHECK( 75.0f == current_daylight_level( winter ) );
    }

    // Many other times of day have peak daylight level, but noon is for sure
    SECTION( "noon is peak daylight level" ) {
        CHECK( 100.0f == sunlight( spring + 12_hours ) );
        CHECK( 125.0f == sunlight( summer + 12_hours ) );
        CHECK( 100.0f == sunlight( autumn + 12_hours ) );
        CHECK( 75.0f == sunlight( winter + 12_hours ) );
    }
}

// The times of sunrise and sunset vary throughout the year. For simplicity, equinoxes occur on the
// first day of spring and autumn, and solstices occur on the first day of summer and winter.
TEST_CASE( "sunrise and sunset", "[sun][sunrise][sunset][equinox][solstice]" )
{
    // Due to the "NN_days" math below, this test requires a default 91-day season length
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    static const time_duration one_season = calendar::season_length();
    static const time_point spring = calendar::turn_zero;
    static const time_point summer = spring + one_season;
    static const time_point autumn = summer + one_season;
    static const time_point winter = autumn + one_season;

    // The expected sunrise/sunset times depend on internal values in `calendar.cpp` including:
    // - sunrise_winter, sunrise_summer, sunrise_equinox
    // - sunset_winter, sunset_summer, sunset_equinox
    // These being constants based on the default game setting in New England, planet Earth.
    // Were these to become variable, the tests would need to adapt.

    SECTION( "spring equinox is day 1 of spring" ) {
        // 11 hours of daylight
        CHECK( "Year 1, Spring, day 1 6:00:00 AM" == to_string( sunrise( spring ) ) );
        CHECK( "Year 1, Spring, day 1 7:00:00 PM" == to_string( sunset( spring ) ) );

        THEN( "sunrise gets earlier" ) {
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( spring ) ) );
            CHECK( "5:40:00 AM" == to_string_time_of_day( sunrise( spring + 30_days ) ) );
            CHECK( "5:20:00 AM" == to_string_time_of_day( sunrise( spring + 60_days ) ) );
            CHECK( "5:00:00 AM" == to_string_time_of_day( sunrise( spring + 90_days ) ) );
        }
        THEN( "sunset gets later" ) {
            CHECK( "7:00:00 PM" == to_string_time_of_day( sunset( spring ) ) );
            CHECK( "7:39:00 PM" == to_string_time_of_day( sunset( spring + 30_days ) ) );
            CHECK( "8:19:00 PM" == to_string_time_of_day( sunset( spring + 60_days ) ) );
            CHECK( "8:58:00 PM" == to_string_time_of_day( sunset( spring + 90_days ) ) );
        }
    }

    SECTION( "summer solstice is day 1 of summer" ) {
        // 14 hours of daylight
        CHECK( "Year 1, Summer, day 1 5:00:00 AM" == to_string( sunrise( summer ) ) );
        CHECK( "Year 1, Summer, day 1 9:00:00 PM" == to_string( sunset( summer ) ) );

        THEN( "sunrise gets later" ) {
            CHECK( "5:00:00 AM" == to_string_time_of_day( sunrise( summer ) ) );
            CHECK( "5:19:00 AM" == to_string_time_of_day( sunrise( summer + 30_days ) ) );
            CHECK( "5:39:00 AM" == to_string_time_of_day( sunrise( summer + 60_days ) ) );
            CHECK( "5:59:00 AM" == to_string_time_of_day( sunrise( summer + 90_days ) ) );
        }
        THEN( "sunset gets earlier" ) {
            CHECK( "9:00:00 PM" == to_string_time_of_day( sunset( summer ) ) );
            CHECK( "8:20:00 PM" == to_string_time_of_day( sunset( summer + 30_days ) ) );
            CHECK( "7:40:00 PM" == to_string_time_of_day( sunset( summer + 60_days ) ) );
            CHECK( "7:01:00 PM" == to_string_time_of_day( sunset( summer + 90_days ) ) );
        }
    }

    SECTION( "autumn equinox is day 1 of autumn" ) {
        // 11 hours of daylight
        CHECK( "Year 1, Autumn, day 1 6:00:00 AM" == to_string( sunrise( autumn ) ) );
        CHECK( "Year 1, Autumn, day 1 7:00:00 PM" == to_string( sunset( autumn ) ) );

        THEN( "sunrise gets later" ) {
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( autumn ) ) );
            CHECK( "6:19:00 AM" == to_string_time_of_day( sunrise( autumn + 30_days ) ) );
            CHECK( "6:39:00 AM" == to_string_time_of_day( sunrise( autumn + 60_days ) ) );
            CHECK( "6:59:00 AM" == to_string_time_of_day( sunrise( autumn + 90_days ) ) );
        }
        THEN( "sunset gets earlier" ) {
            CHECK( "7:00:00 PM" == to_string_time_of_day( sunset( autumn ) ) );
            CHECK( "6:20:00 PM" == to_string_time_of_day( sunset( autumn + 30_days ) ) );
            CHECK( "5:40:00 PM" == to_string_time_of_day( sunset( autumn + 60_days ) ) );
            CHECK( "5:01:00 PM" == to_string_time_of_day( sunset( autumn + 90_days ) ) );
        }
    }

    SECTION( "winter solstice is day 1 of winter" ) {
        // 10 hours of daylight
        CHECK( "Year 1, Winter, day 1 7:00:00 AM" == to_string( sunrise( winter ) ) );
        CHECK( "Year 1, Winter, day 1 5:00:00 PM" == to_string( sunset( winter ) ) );

        THEN( "sunrise gets earlier" ) {
            CHECK( "7:00:00 AM" == to_string_time_of_day( sunrise( winter ) ) );
            CHECK( "6:40:00 AM" == to_string_time_of_day( sunrise( winter + 30_days ) ) );
            CHECK( "6:20:00 AM" == to_string_time_of_day( sunrise( winter + 60_days ) ) );
            CHECK( "6:00:00 AM" == to_string_time_of_day( sunrise( winter + 90_days ) ) );
        }
        THEN( "sunset gets later" ) {
            CHECK( "5:00:00 PM" == to_string_time_of_day( sunset( winter ) ) );
            CHECK( "5:39:00 PM" == to_string_time_of_day( sunset( winter + 30_days ) ) );
            CHECK( "6:19:00 PM" == to_string_time_of_day( sunset( winter + 60_days ) ) );
            CHECK( "6:58:00 PM" == to_string_time_of_day( sunset( winter + 90_days ) ) );
        }
    }
}

static rl_vec2d checked_sunlight_angle( const time_point &t )
{
    const cata::optional<rl_vec2d> opt_angle = sunlight_angle( t );
    REQUIRE( opt_angle );
    return *opt_angle;
}

static constexpr time_point first_midnight = calendar::turn_zero;
static constexpr time_point first_noon = first_midnight + 12_hours;

TEST_CASE( "sun_highest_at_noon", "[sun]" )
{
    for( int i = 0; i < 100; ++i ) {
        CAPTURE( i );

        const time_point midnight = first_midnight + i * 1_days;
        CHECK_FALSE( sunlight_angle( midnight ) );

        const time_point noon = first_noon + i * 1_days;
        const time_point before_noon = noon - 2_hours;
        const time_point after_noon = noon + 2_hours;

        const rl_vec2d before_noon_angle = checked_sunlight_angle( before_noon );
        const rl_vec2d noon_angle = checked_sunlight_angle( noon );
        const rl_vec2d after_noon_angle = checked_sunlight_angle( after_noon );

        CAPTURE( before_noon_angle );
        CAPTURE( noon_angle );
        CAPTURE( after_noon_angle );
        // Sun should be highest around noon
        CHECK( noon_angle.magnitude() < before_noon_angle.magnitude() );
        CHECK( noon_angle.magnitude() < after_noon_angle.magnitude() );

        // Sun should always be in the South, meaning angle points North
        // (negative)
        CHECK( before_noon_angle.y < 0 );
        CHECK( noon_angle.y < 0 );
        CHECK( after_noon_angle.y < 0 );

        // Sun should be moving westwards across the sky, so its angle points
        // more eastwards, which means it's increasing
        CHECK( noon_angle.x > before_noon_angle.x );
        CHECK( after_noon_angle.x > noon_angle.x );

        CHECK( before_noon_angle.magnitude() ==
               Approx( after_noon_angle.magnitude() ).epsilon( 0.25 ) );
    }
}

TEST_CASE( "noon_sun_doesn't_move_much", "[sun]" )
{
    rl_vec2d noon_angle = checked_sunlight_angle( first_noon );
    for( int i = 1; i < 1000; ++i ) {
        CAPTURE( i );
        const time_point later_noon = first_noon + i * 1_days;
        const rl_vec2d later_noon_angle = checked_sunlight_angle( later_noon );
        CHECK( noon_angle.x == Approx( later_noon_angle.x ).margin( 0.01 ) );
        CHECK( noon_angle.y == Approx( later_noon_angle.y ).epsilon( 0.05 ) );
        noon_angle = later_noon_angle;
    }
}

using PointSet = std::unordered_set<std::pair<int, int>, cata::tuple_hash>;

static PointSet sun_positions_regular( time_point start, time_point end, time_duration interval,
                                       int azimuth_scale )
{
    CAPTURE( to_days<int>( start - calendar::turn_zero ) );
    std::unordered_set<std::pair<int, int>, cata::tuple_hash> plot_points;

    for( time_point t = start; t < end; t += interval ) {
        CAPTURE( to_minutes<int>( t - start ) );
        units::angle azimuth, altitude;
        std::tie( azimuth, altitude ) = sun_azimuth_altitude( t, location_boston, -5 );
        if( altitude < 0_degrees ) {
            continue;
        }
        // Convert to ASCII-art plot
        // x-axis is azimuth, 4/azimuth_scale degrees per column
        // y-axis is altitude, 3 degrees per column
        azimuth = normalize( azimuth + 180_degrees );
        // Scale azimuth away from 180 by specified scale
        azimuth = 180_degrees + ( azimuth - 180_degrees ) * azimuth_scale;
        REQUIRE( azimuth >= 0_degrees );
        REQUIRE( azimuth <= 360_degrees );
        REQUIRE( altitude >= 0_degrees );
        REQUIRE( altitude <= 90_degrees );
        plot_points.emplace( azimuth / 4_degrees, altitude / 3_degrees );
    }

    return plot_points;
}

static PointSet sun_throughout_day( time_point day_start )
{
    REQUIRE( time_past_midnight( day_start ) == 0_seconds );
    // Calculate the Sun's position every few minutes thourhgout the day
    time_point day_end = day_start + 1_days;
    return sun_positions_regular( day_start, day_end, 5_minutes, 1 );
}

static PointSet sun_throughout_year( time_point day_start )
{
    REQUIRE( time_past_midnight( day_start ) == 0_seconds );
    // Calculate the Sun's position every noon throughout the year
    time_point first_noon = day_start + 1_days / 2;
    time_point last_noon = first_noon + calendar::year_length();
    return sun_positions_regular( first_noon, last_noon, 1_days, 4 );
}

static void check_sun_plot( const std::vector<PointSet> &points, const std::string &reference )
{
    static constexpr std::array<char, 3> symbols = { '#', '*', '.' };
    REQUIRE( points.size() <= symbols.size() );

    std::ostringstream os;
    os << "Altitude\n";

    for( int rough_altitude = 30; rough_altitude >= 0; --rough_altitude ) {
        for( int rough_azimuth = 0; rough_azimuth <= 90; ++rough_azimuth ) {
            std::pair<int, int> p{ rough_azimuth, rough_altitude };
            char c = ' ';
            for( size_t i = 0; i < points.size(); ++i ) {
                if( points[i].count( p ) ) {
                    c = symbols[i];
                    break;
                }
            }
            os << c;
        }
        os << '\n';
    }
    os << std::setw( 92 ) << "Azimuth\n";
    std::string result = os.str();
    CHECK( result == reference );
    // When the test fails, print out something to copy-paste as a new
    // reference output:
    if( result != reference ) {
        result.pop_back();
        for( const std::string &line : string_split( result, '\n' ) ) {
            printf( R"("%s\n")" "\n", line.c_str() );
        }
    }
}

TEST_CASE( "movement_of_sun_through_day", "[sun]" )
{
    PointSet equinox_points = sun_throughout_day( calendar::turn_zero );
    PointSet summer_points =
        sun_throughout_day( calendar::turn_zero + calendar::season_length() );
    PointSet winter_points =
        sun_throughout_day( calendar::turn_zero + calendar::season_length() * 3 );
    std::string reference =
// *INDENT-OFF*
"Altitude\n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                      ##############                                       \n"
"                                  ####              ####                                   \n"
"                                ##                      ##                                 \n"
"                              ##                          ##                               \n"
"                            ##                              ##                             \n"
"                           ##                                ##                            \n"
"                          ##                                  ##                           \n"
"                         ##               ******               ##                          \n"
"                        ##            ****      ****            ##                         \n"
"                       ##          ***              ***          #                         \n"
"                       #          **                  ***         #                        \n"
"                      #         **                      **         #                       \n"
"                     ##        **                         *        ##                      \n"
"                     #        *                            *        #                      \n"
"                    #        *                              *       ##                     \n"
"                   ##       *              ....              *       ##                    \n"
"                   #       *           .....  .....           *       #                    \n"
"                  ##      **         ...          ...         **      ##                   \n"
"                  #      **        ...              ...        **      #                   \n"
"                 #       *        ..                  ..        *       #                  \n"
"                ##      *        ..                    ..        *      ##                 \n"
"               ##      **       ..                      ..       **      ##                \n"
"               #      **       ..                        ..       **      #                \n"
"              ##      *       ..                          ..       *      ##               \n"
"                                                                                    Azimuth\n";
// *INDENT-ON*
    check_sun_plot( { summer_points, equinox_points, winter_points }, reference );
}

TEST_CASE( "movement_of_noon_through_year", "[sun]" )
{
    PointSet points = sun_throughout_year( calendar::turn_zero );
    std::string reference =
// *INDENT-OFF*
// This should yield an analemma
"Altitude\n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                             #######                                       \n"
"                                             #     #                                       \n"
"                                             #    ##                                       \n"
"                                             ##  ##                                        \n"
"                                              ####                                         \n"
"                                              ###                                          \n"
"                                             ## ##                                         \n"
"                                             #   ##                                        \n"
"                                            #     #                                        \n"
"                                           ##     ##                                       \n"
"                                           #       #                                       \n"
"                                          ##       #                                       \n"
"                                          #        #                                       \n"
"                                          ##       #                                       \n"
"                                           ##     ##                                       \n"
"                                            #######                                        \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                           \n"
"                                                                                    Azimuth\n";
// *INDENT-ON*
    check_sun_plot( { points }, reference );
}

TEST_CASE( "noon_rises_towards_solsitice", "[sun]" )
{
    static constexpr time_point first_noon = calendar::turn_zero + 12_hours;
    const time_point end_of_spring = calendar::turn_zero + calendar::season_length();
    rl_vec2d last_noon_angle;

    for( time_point noon = first_noon; noon < end_of_spring; noon += 1_days ) {
        CAPTURE( to_days<int>( noon - first_noon ) );

        const rl_vec2d noon_angle = checked_sunlight_angle( noon );

        if( last_noon_angle.magnitude() != 0 ) {
            CAPTURE( last_noon_angle );
            CAPTURE( noon_angle );

            // Sun should be higher than yesterday
            CHECK( noon_angle.magnitude() < last_noon_angle.magnitude() );
        }

        last_noon_angle = noon_angle;
    }
}
