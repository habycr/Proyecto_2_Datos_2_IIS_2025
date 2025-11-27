#include "ProblemRepository.h"

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::array;

// =================== Helpers internos ===================

// Convierte un campo string BSON a std::string usando get_string() y k_string
static std::string get_string_field(const bsoncxx::document::view& doc,
                                    const char* field_name) {
    auto elem = doc[field_name];
    if (elem && elem.type() == bsoncxx::type::k_string) {
        auto sv = elem.get_string().value;           // string_view
        return std::string(sv.data(), sv.size());    // lo convertimos a std::string
    }
    return {};
}

// Convierte un documento BSON a un struct Problem
static Problem document_to_problem(const bsoncxx::document::view& doc_view) {
    Problem p;

    // Campos simples
    p.problem_id  = get_string_field(doc_view, "problem_id");
    p.title       = get_string_field(doc_view, "title");
    p.description = get_string_field(doc_view, "description");
    p.difficulty  = get_string_field(doc_view, "difficulty");
    p.code_stub   = get_string_field(doc_view, "code_stub");

    // tags: arreglo de strings
    auto it_tags = doc_view.find("tags");
    if (it_tags != doc_view.end() && it_tags->type() == bsoncxx::type::k_array) {
        auto arr = it_tags->get_array().value;
        for (auto&& t : arr) {
            if (t.type() == bsoncxx::type::k_string) {
                auto sv = t.get_string().value;
                p.tags.emplace_back(sv.data(), sv.size());
            }
        }
    }

    // test_cases: arreglo de documentos { input, expected_output }
    auto it_tcs = doc_view.find("test_cases");
    if (it_tcs != doc_view.end() && it_tcs->type() == bsoncxx::type::k_array) {
        auto arr = it_tcs->get_array().value;
        for (auto&& tc_elem : arr) {
            if (tc_elem.type() == bsoncxx::type::k_document) {
                auto tc_doc = tc_elem.get_document().value;
                TestCase tc;

                auto it_in = tc_doc.find("input");
                if (it_in != tc_doc.end() && it_in->type() == bsoncxx::type::k_string) {
                    auto sv = it_in->get_string().value;
                    tc.input.assign(sv.data(), sv.size());
                }

                auto it_out = tc_doc.find("expected_output");
                if (it_out != tc_doc.end() && it_out->type() == bsoncxx::type::k_string) {
                    auto sv = it_out->get_string().value;
                    tc.expected_output.assign(sv.data(), sv.size());
                }

                p.test_cases.push_back(std::move(tc));
            }
        }
    }

    return p;
}

// =================== Implementación de ProblemRepository ===================

ProblemRepository::ProblemRepository(mongocxx::database db)
    : collection_{db["problems"]} {}

// INSERT
void ProblemRepository::insert_problem(const Problem& p) {
    // Construir arreglo de tags
    array tags_arr;
    for (const auto& t : p.tags) {
        tags_arr.append(t);
    }

    // Construir arreglo de test_cases
    array tcs_arr;
    for (const auto& tc : p.test_cases) {
        document tc_doc;
        tc_doc.append(
            kvp("input", tc.input),
            kvp("expected_output", tc.expected_output)
        );
        tcs_arr.append(tc_doc.extract());
    }

    document doc_builder;
    doc_builder.append(
        kvp("problem_id", p.problem_id),
        kvp("title", p.title),
        kvp("description", p.description),
        kvp("difficulty", p.difficulty),
        kvp("code_stub", p.code_stub),
        kvp("tags", tags_arr),
        kvp("test_cases", tcs_arr)
    );

    collection_.insert_one(doc_builder.view());
}

// GET ALL
std::vector<Problem> ProblemRepository::get_all() {
    std::vector<Problem> results;
    auto cursor = collection_.find({});
    for (auto&& doc : cursor) {
        results.push_back(document_to_problem(doc));
    }
    return results;
}

// GET BY ID (problem_id)
std::optional<Problem> ProblemRepository::get_by_id(const std::string& id) {
    auto filter_doc = document{};
    filter_doc.append(kvp("problem_id", id));

    auto maybe_doc = collection_.find_one(filter_doc.view());
    if (!maybe_doc) {
        return std::nullopt;
    }

    return document_to_problem(maybe_doc->view());
}

// GET BY DIFFICULTY
std::vector<Problem> ProblemRepository::get_by_difficulty(const std::string& difficulty) {
    std::vector<Problem> results;

    document filter_doc;
    filter_doc.append(kvp("difficulty", difficulty));

    auto cursor = collection_.find(filter_doc.view());
    for (auto&& doc : cursor) {
        results.push_back(document_to_problem(doc));
    }

    return results;
}

// UPDATE (por problem_id)
bool ProblemRepository::update_problem(const Problem& p) {
    // tags
    array tags_arr;
    for (const auto& t : p.tags) {
        tags_arr.append(t);
    }

    // test_cases
    array tcs_arr;
    for (const auto& tc : p.test_cases) {
        document tc_doc;
        tc_doc.append(
            kvp("input", tc.input),
            kvp("expected_output", tc.expected_output)
        );
        tcs_arr.append(tc_doc.extract());
    }

    // Documento con los campos a actualizar
    document set_doc;
    set_doc.append(
        kvp("title", p.title),
        kvp("description", p.description),
        kvp("difficulty", p.difficulty),
        kvp("code_stub", p.code_stub),
        kvp("tags", tags_arr),
        kvp("test_cases", tcs_arr)
    );

    // Filtro por problem_id
    document filter_doc;
    filter_doc.append(kvp("problem_id", p.problem_id));

    document update_doc;
    update_doc.append(kvp("$set", set_doc.view()));

    auto result = collection_.update_one(filter_doc.view(), update_doc.view());
    return result && result->modified_count() > 0;
}

// DELETE (por problem_id)
bool ProblemRepository::delete_problem(const std::string& id) {
    document filter_doc;
    filter_doc.append(kvp("problem_id", id));

    auto result = collection_.delete_one(filter_doc.view());
    return result && result->deleted_count() > 0;
}
