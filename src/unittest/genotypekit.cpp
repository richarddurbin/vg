/**
 * unittest/genotypekit.cpp: test cases for genotypekit modular genotyper pieces
 */

#include "catch.hpp"
#include "genotypekit.hpp"

namespace vg {
namespace unittest {

TEST_CASE("sites can be found with Cactus", "[genotype]") {
    
    // Build a toy graph
    const string graph_json = R"(
    
    {
        "node": [
            {"id": 1, "sequence": "G"},
            {"id": 2, "sequence": "A"},
            {"id": 3, "sequence": "T"},
            {"id": 4, "sequence": "GGG"},
            {"id": 5, "sequence": "T"},
            {"id": 6, "sequence": "A"},
            {"id": 7, "sequence": "C"},
            {"id": 8, "sequence": "A"},
            {"id": 9, "sequence": "A"}
        ],
        "edge": [
            {"from": 1, "to": 2},
            {"from": 1, "to": 6},
            {"from": 2, "to": 3},
            {"from": 2, "to": 4},
            {"from": 3, "to": 5},
            {"from": 4, "to": 5},
            {"from": 5, "to": 6},
            {"from": 6, "to": 7},
            {"from": 6, "to": 8},
            {"from": 7, "to": 9},
            {"from": 8, "to": 9}
            
        ],
        "path": [
            {"name": "hint", "mapping": [
                {"position": {"node_id": 1}},
                {"position": {"node_id": 6}},
                {"position": {"node_id": 8}},
                {"position": {"node_id": 9}}
            ]}
        ]
    }
    
    )";
    
    // Make an actual graph
    VG graph;
    Graph chunk;
    json2pb(chunk, graph_json.c_str(), graph_json.size());
    graph.merge(chunk);
    
    // Make a CactusSiteFinder
    SiteFinder* finder = new CactusSiteFinder(graph, "hint");
    
    SECTION("CactusSiteFinder should find two top-level sites") {
        vector<NestedSite> sites;
        finder->for_each_site_parallel([&](const NestedSite& site) {
            // Just sum up the top-level sites
            #pragma omp crirical (sites)
            sites.push_back(site);
        });
        
        REQUIRE(sites.size() == 2);
        
        // Now sort them
        sort(sites.begin(), sites.end(), [](const NestedSite& a, const NestedSite& b) {
            // TODO: add a comparison to NestedSite.
            return make_pair(a.start, a.end) < make_pair(b.start, b.end);
        });
        
        SECTION("the first site should be 1 fwd to 6 fwd") {
            REQUIRE(sites[0].start.node->id() == 1);
            REQUIRE(sites[0].start.backward == false);
            REQUIRE(sites[0].end.node->id() == 6);
            REQUIRE(sites[0].end.backward == false);
            
            SECTION("and should contain exactly nodes 1 and 6") {
                auto& nodes = sites[0].nodes;
                REQUIRE(nodes.count(graph.get_node(1)) == 1);
                REQUIRE(nodes.count(graph.get_node(6)) == 1);
                REQUIRE(nodes.size() == 2);
            }
            
            SECTION("and should contain exactly edges 1->6, 1->2, and 5->6") {
                auto& edges = sites[0].edges;
                REQUIRE(edges.count(graph.get_edge(NodeSide(1, true), NodeSide(6))) == 1);
                REQUIRE(edges.count(graph.get_edge(NodeSide(1, true), NodeSide(2))) == 1);
                REQUIRE(edges.count(graph.get_edge(NodeSide(5, true), NodeSide(6))) == 1);
                REQUIRE(edges.size() == 3);
            }
            
            SECTION("and should contain one child") {
                REQUIRE(sites[0].children.size() == 1);
                
                auto& child = sites[0].children[0];
                
                SECTION("that child should be 2 fwd to 5 fwd") {
                    REQUIRE(child.start.node->id() == 1);
                    REQUIRE(child.start.backward == false);
                    REQUIRE(child.end.node->id() == 6);
                    REQUIRE(child.end.backward == false);
                }
            }
            
        }
        
        SECTION("the second site should be 6 fwd to 9 fwd") {
            REQUIRE(sites[1].start.node->id() == 6);
            REQUIRE(sites[1].start.backward == false);
            REQUIRE(sites[1].end.node->id() == 9);
            REQUIRE(sites[1].end.backward == false);
            
            SECTION("and should contain no children") {
                REQUIRE(sites[1].children.size() == 0);
            }
        }
        
        
    }
    
    delete finder;

}

TEST_CASE("fixed priors can be assigned to genotypes", "[genotype]") {
    
    GenotypePriorCalculator* calculator = new FixedGenotypePriorCalculator();
    
    Genotype het;
    het.add_allele(0);
    het.add_allele(1);
    
    Genotype hom_alt;
    hom_alt.add_allele(1);
    hom_alt.add_allele(1);
    
    Genotype hom_ref;
    hom_ref.add_allele(0);
    hom_ref.add_allele(0);
    
    SECTION("homozygote priors should be equal") {
        REQUIRE(calculator->calculate_log_prior(hom_alt) == calculator->calculate_log_prior(hom_ref));
    }
    
    SECTION("homozygotes should be more likely than heterozygotes") {
        REQUIRE(calculator->calculate_log_prior(het) < calculator->calculate_log_prior(hom_ref));
        REQUIRE(calculator->calculate_log_prior(het) < calculator->calculate_log_prior(hom_alt));
    }
    
    SECTION("haploid genotypes should have nonzero prior") {
        Genotype haploid;
        haploid.add_allele(5);
        REQUIRE(calculator->calculate_log_prior(haploid) > prob_to_logprob(0));
    }
    
    SECTION("zero-ploid genotypes should have nonzero prior") {
        Genotype empty;
        REQUIRE(calculator->calculate_log_prior(empty) > prob_to_logprob(0));
    }
    
    SECTION("polyploid genotypes should have nonzero prior") {
        Genotype polyploid;
        for(int i = 0; i < 100; i++) {
            polyploid.add_allele(i);
        }
        REQUIRE(calculator->calculate_log_prior(polyploid) > prob_to_logprob(0));
    }
    
    delete calculator;
}

}
}
