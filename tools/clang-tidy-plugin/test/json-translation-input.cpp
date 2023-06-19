// RUN: %check_clang_tidy -allow-stdinc %s cata-json-translation-input %t -- --load=%cata_plugin -- -isystem %cata_include -isystem %cata_include/third-party -DLOCALIZE

#include "json.h"
#include "translations.h"

class foo
{
    public:
        std::string name;
        std::string double_name;
        std::string nickname;
        translation msg;
        translation more_msg;
        translation moar_msg;
        translation endless_msg;
        translation desc;
        std::string whatever;
};

// <translation function>( ... <json input object>.<method>(...) ... )
static void deserialize( foo &bar, JsonObject &jo )
{
    bar.name = _( jo.get_string( "name" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:16: warning: immediately translating a value read from json causes translation updating issues.  Consider reading into a translation object instead.
    // CHECK-MESSAGES: [[@LINE-2]]:19: note: value read from json

    JsonArray ja = jo.get_array( "double_name" );
    bar.double_name = std::string( _( ja.next_string() ) ) + pgettext( "blah",
                      // CHECK-MESSAGES: [[@LINE-1]]:36: warning: immediately translating a value read from json causes translation updating issues.  Consider reading into a translation object instead.
                      // CHECK-MESSAGES: [[@LINE-2]]:39: note: value read from json
                      // CHECK-MESSAGES: [[@LINE-3]]:62: warning: immediately translating a value read from json causes translation updating issues.  Consider reading into a translation object instead.
                      std::string( ja.next_string() ).c_str() );
    // CHECK-MESSAGES: [[@LINE-1]]:36: note: value read from json

    // ok, not reading from json
    bar.nickname = _( "blah" );

    bar.msg = to_translation( jo.get_string( "message" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:15: warning: read translation directly instead of constructing it from json strings to ensure consistent format in json.
    // CHECK-MESSAGES: [[@LINE-2]]:31: note: string read from json

    bar.more_msg = pl_translation( jo.get_string( "message" ), "foo" );
    // CHECK-MESSAGES: [[@LINE-1]]:20: warning: read translation directly instead of constructing it from json strings to ensure consistent format in json.
    // CHECK-MESSAGES: [[@LINE-2]]:36: note: string read from json

    bar.moar_msg = translation::to_translation( jo.get_string( "more_message" ) );
    // CHECK-MESSAGES: [[@LINE-1]]:20: warning: read translation directly instead of constructing it from json strings to ensure consistent format in json.
    // CHECK-MESSAGES: [[@LINE-2]]:49: note: string read from json

    // ok, reading translation directly
    jo.read( "endless_msg", bar.endless_msg );

    // ok, using no_translation to avoid re-translating generated string
    bar.desc = no_translation( jo.get_string( "generated_desc" ) );

    std::string duh;
    // ok, not reading from json
    bar.whatever = _( duh.c_str() );
}
