#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("substr") {
    struct Test {
        std::string text;
        int x = 0;
        int y = 0;
    };
    auto storage = make_storage(
        {},
        make_table("test", make_column("text", &Test::text), make_column("x", &Test::x), make_column("y", &Test::y)));
    storage.sync_schema();

    {
        auto rows = storage.select(substr("SQLite substr", 8));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        storage.insert(Test{"SQLite substr", 8});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        auto rows = storage.select(substr("SQLite substr", 1, 6));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
    {

        storage.remove_all<Test>();
        storage.insert(Test{"SQLite substr", 1, 6});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x, &Test::y));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
}

TEST_CASE("zeroblob") {
    struct Test {
        int value = 0;
    };

    auto storage = make_storage({}, make_table("test", make_column("value", &Test::value)));
    storage.sync_schema();

    {
        auto rows = storage.select(zeroblob(10));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 10);
        std::vector<char> expectedValue(10);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
    {
        storage.insert(Test{100});

        auto rows = storage.select(zeroblob(&Test::value));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 100);
        std::vector<char> expectedValue(100);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
}

TEST_CASE("julianday") {
    struct Test {
        std::string text;
    };

    auto storage = make_storage({}, make_table("test", make_column("text", &Test::text)));
    storage.sync_schema();
    auto singleTestCase = [&storage](const std::string &arg, double expected) {
        {
            auto rows = storage.select(julianday(arg));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001);  //  too much precision
        }
        {
            storage.insert(Test{arg});
            auto rows = storage.select(julianday(&Test::text));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001);
            storage.remove_all<Test>();
        }
    };
    singleTestCase("2016-10-18", 2457679.5);
    singleTestCase("2016-10-18 16:45", 2457680.19791667);
    singleTestCase("2016-10-18 16:45:30", 2457680.19826389);
}

TEST_CASE("datetime") {
    auto storage = make_storage({});
    auto rows = storage.select(datetime("now"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

TEST_CASE("date") {
    auto storage = make_storage({});
    auto rows = storage.select(date("now", "start of month", "+1 month", "-1 day"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

#if SQLITE_VERSION_NUMBER >= 3007016
TEST_CASE("char") {
    auto storage = make_storage({});
    auto rows = storage.select(char_(67, 72, 65, 82));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "CHAR");
}
#endif

TEST_CASE("rtrim") {
    auto storage = make_storage({});
    auto rows = storage.select(rtrim("ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(rtrim("ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("ltrim") {
    auto storage = make_storage({});
    auto rows = storage.select(ltrim("  ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(ltrim("  ototo", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("trim") {
    auto storage = make_storage({});
    auto rows = storage.select(trim("   ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");

    rows = storage.select(trim("   ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("upper") {
    auto storage = make_storage({});
    auto rows = storage.select(upper("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "OTOTO");
}

TEST_CASE("lower") {
    auto storage = make_storage({});
    auto rows = storage.select(lower("OTOTO"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("length") {
    auto storage = make_storage({});
    auto rows = storage.select(length("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 5);
}

TEST_CASE("abs") {
    auto storage = make_storage({});
    auto rows = storage.select(sqlite_orm::abs(-10));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front());
    REQUIRE(*rows.front() == 10);
}

TEST_CASE("hex") {
    auto storage = make_storage({});
    {
        auto rows = storage.select(hex(67));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "3637");
    }
    {
        auto rows = storage.select(hex("ä"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "C3A4");
    }
    {
        auto rows = storage.select(hex(nullptr));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == std::string());
    }
}

TEST_CASE("quote") {
    using Catch::Matchers::UnorderedEquals;
    struct Department {
        int id = 0;
        std::string name;
        int managerId = 0;
        int locationId = 0;
    };
    auto storage = make_storage({},
                                make_table("departments",
                                           make_column("department_id", &Department::id, primary_key()),
                                           make_column("department_name", &Department::name),
                                           make_column("manager_id", &Department::managerId),
                                           make_column("location_id", &Department::locationId)));
    storage.sync_schema();
    storage.replace(Department{10, "Administration", 200, 1700});
    storage.replace(Department{20, "Marketing", 201, 1800});
    storage.replace(Department{30, "Purchasing", 114, 1700});
    storage.replace(Department{40, "Human Resources", 203, 2400});
    storage.replace(Department{50, "Shipping", 121, 1500});
    storage.replace(Department{60, "IT", 103, 1400});
    storage.replace(Department{70, "Public Relation", 204, 2700});
    storage.replace(Department{80, "Sales", 145, 2500});
    storage.replace(Department{90, "Executive", 100, 1700});
    storage.replace(Department{100, "Finance", 108, 1700});
    storage.replace(Department{110, "Accounting", 205, 1700});
    storage.replace(Department{120, "Treasury", 0, 1700});
    storage.replace(Department{130, "Corporate Tax", 0, 1700});
    storage.replace(Department{140, "Control And Cre", 0, 1700});
    storage.replace(Department{150, "Shareholder Ser", 0, 1700});
    storage.replace(Department{160, "Benefits", 0, 1700});
    storage.replace(Department{170, "Manufacturing", 0, 1700});
    storage.replace(Department{180, "Construction", 0, 1700});
    storage.replace(Department{190, "Contracting", 0, 1700});
    storage.replace(Department{200, "Operations", 0, 1700});
    storage.replace(Department{210, "IT Support", 0, 1700});
    storage.replace(Department{220, "NOC", 0, 1700});
    storage.replace(Department{230, "IT Helpdesk", 0, 1700});
    storage.replace(Department{240, "Government Sale", 0, 1700});
    storage.replace(Department{250, "Retail Sales", 0, 1700});
    storage.replace(Department{260, "Recruiting", 0, 1700});
    storage.replace(Department{270, "Payroll", 0, 1700});
    {
        auto rows = storage.select(quote("hi"));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "'hi'");
    }
    {
        auto rows =
            storage.select(columns(&Department::name, quote(&Department::name)), where(c(&Department::id) > 150));
        std::vector<std::tuple<std::string, std::string>> expected;
        expected.push_back(std::make_tuple("Benefits", "'Benefits'"));
        expected.push_back(std::make_tuple("Manufacturing", "'Manufacturing'"));
        expected.push_back(std::make_tuple("Construction", "'Construction'"));
        expected.push_back(std::make_tuple("Contracting", "'Contracting'"));
        expected.push_back(std::make_tuple("Operations", "'Operations'"));
        expected.push_back(std::make_tuple("IT Support", "'IT Support'"));
        expected.push_back(std::make_tuple("NOC", "'NOC'"));
        expected.push_back(std::make_tuple("IT Helpdesk", "'IT Helpdesk'"));
        expected.push_back(std::make_tuple("Government Sale", "'Government Sale'"));
        expected.push_back(std::make_tuple("Retail Sales", "'Retail Sales'"));
        expected.push_back(std::make_tuple("Recruiting", "'Recruiting'"));
        expected.push_back(std::make_tuple("Payroll", "'Payroll'"));
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}

TEST_CASE("randomblob") {
    auto storage = make_storage({});
    for(auto i = 0; i < 20; ++i) {
        auto blobLength = i + 1;
        auto rows = storage.select(randomblob(blobLength));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().size() == size_t(blobLength));
    }
}
