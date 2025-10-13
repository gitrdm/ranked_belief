def test_package_imports_and_exposes_version():
    import ranked_belief

    assert ranked_belief.__version__ == "0.1.0"
