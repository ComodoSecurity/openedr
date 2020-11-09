/** Basic Info **

Copyright: 2019 Johnny Hendriks

Author : Johnny Hendriks
Year   : 2019
Project: VSTestAdapter for Catch2
Licence: MIT

Notes: None

** Basic Info **/

#ifndef TESTADAPTER_CATCH_DISCOVER_HPP_INCLUDED
#define TESTADAPTER_CATCH_DISCOVER_HPP_INCLUDED

// Don't #include any Catch headers here - we can assume they are already
// included before this header.
// This is not good practice in general but is necessary in this case so this
// file can be distributed as a single header that works with the main
// Catch single header.

namespace Catch
{

    // Declarations
    void addDiscoverOption(Session& session, bool& doDiscover);
    int  runDiscoverSession(Session& session, bool& doDiscover);
    void discoverTests(Catch::Session& session);

    class DiscoverReporter;

    // Definitions
    void addDiscoverOption(Session& session, bool& doDiscover)
    {
        using namespace Catch::clara;

        auto cli = session.cli()
            | Opt(doDiscover)
              ["--discover"]
              ("Perform VS Test Adaptor discovery");

        session.cli(cli);
    }

    int runDiscoverSession(Session& session, bool& doDiscover)
    {
        if(doDiscover)
        {
            try
            {
                discoverTests(session);
                return 0;
            }
            catch( std::exception& ex )
            {
                cerr() << ex.what() << std::endl;
                return 255;
            }
        }

        return session.run();
    }

    void discoverTests(Catch::Session& session)
    {
        // Retrieve testcases
        const auto& config = session.config();
        auto testspec = config.testSpec();
        auto testcases = filterTests( Catch::getAllTestCasesSorted(config)
                                    , testspec
                                    , config );

        // Setup reporter
        TestRunInfo runInfo(config.name());

        auto pConfig = std::make_shared<Config const>(session.configData());
        auto reporter = getRegistryHub().getReporterRegistry()
                                        .create("discover", pConfig);

        Catch::Totals totals;

        // Start report
        reporter->testRunStarting(runInfo);
        reporter->testGroupStarting(GroupInfo(config.name(), 1, 1));

        // Report test cases
        for (const auto& testcase : testcases)
        {
            Catch::TestCaseInfo caseinfo( testcase.name
                                        , testcase.className
                                        , testcase.description
                                        , testcase.tags
                                        , testcase.lineInfo );
            reporter->testCaseStarting(caseinfo);

        }

        // Close report
        reporter->testGroupEnded(Catch::GroupInfo(config.name(), 1, 1));
        TestRunStats testrunstats(runInfo, totals, false);
        reporter->testRunEnded(testrunstats);
    }

    class DiscoverReporter : public StreamingReporterBase<DiscoverReporter>
    {
        public:
            DiscoverReporter(ReporterConfig const& _config);

            ~DiscoverReporter() override;

            static std::string getDescription();

            virtual std::string getStylesheetRef() const;

            void writeSourceInfo(SourceLineInfo const& sourceInfo);

        public: // StreamingReporterBase


            void testRunStarting(TestRunInfo const& testInfo) override;

            void testGroupStarting(GroupInfo const& groupInfo) override;

            void testCaseStarting(TestCaseInfo const& testInfo) override;

            void assertionStarting(AssertionInfo const&) override;

            bool assertionEnded(AssertionStats const& assertionStats) override;


        private:
            XmlWriter m_xml;
    };

    DiscoverReporter::DiscoverReporter( ReporterConfig const& _config )
      : StreamingReporterBase( _config ),
        m_xml(_config.stream())
    { }

    DiscoverReporter::~DiscoverReporter() = default;

    std::string DiscoverReporter::getDescription()
    {
        return "Reports testcase information as an XML document";
    }

    std::string DiscoverReporter::getStylesheetRef() const
    {
        return std::string();
    }

    void DiscoverReporter::writeSourceInfo( SourceLineInfo const& sourceInfo )
    {
        m_xml.writeAttribute( "filename", sourceInfo.file )
             .writeAttribute( "line", sourceInfo.line );
    }

    void DiscoverReporter::testRunStarting( TestRunInfo const& testInfo )
    {
        StreamingReporterBase::testRunStarting( testInfo );
        std::string stylesheetRef = getStylesheetRef();
        if( !stylesheetRef.empty() )
            m_xml.writeStylesheetRef( stylesheetRef );
        m_xml.startElement( "Catch" );
        if( !m_config->name().empty() )
            m_xml.writeAttribute( "name", m_config->name() );
    }

    void DiscoverReporter::testGroupStarting( GroupInfo const& groupInfo )
    {
        StreamingReporterBase::testGroupStarting( groupInfo );
        m_xml.startElement( "Group" )
            .writeAttribute( "name", groupInfo.name );
    }

    void DiscoverReporter::testCaseStarting( TestCaseInfo const& testInfo )
    {
        StreamingReporterBase::testCaseStarting(testInfo);
        m_xml.startElement( "TestCase" )
             .writeAttribute( "name", testInfo.name )
             .writeAttribute( "description", testInfo.description )
             .writeAttribute( "tags", testInfo.tagsAsString() );

        writeSourceInfo( testInfo.lineInfo );

        m_xml.endElement();
    }

    void DiscoverReporter::assertionStarting( AssertionInfo const& ) { }

    bool DiscoverReporter::assertionEnded( AssertionStats const& )
    {
        return true;
    }

    CATCH_REGISTER_REPORTER( "discover", DiscoverReporter )

}


#endif // TESTADAPTER_CATCH_DISCOVER_HPP_INCLUDED
