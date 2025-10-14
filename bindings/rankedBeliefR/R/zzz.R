.onLoad <- function(libname, pkgname) {
  root <- normalizePath(file.path(system.file(package = pkgname), "..", ".."), winslash = "/", mustWork = FALSE)
  root_env <- Sys.getenv("RANKED_BELIEF_ROOT", unset = NA_character_)

  if (!is.na(root_env) && dir.exists(file.path(root_env, "build"))) {
    return(invisible())
  }

  repo_root <- normalizePath(root_env, winslash = "/", mustWork = FALSE)
  if (is.na(repo_root) || !dir.exists(repo_root)) {
    repo_root <- root
  }

  if (!file.exists(file.path(repo_root, "rp-core.rkt"))) {
    return(invisible())
  }

  build_dir <- normalizePath(file.path(repo_root, "build"), winslash = "/", mustWork = FALSE)
  if (!dir.exists(build_dir)) {
    packageStartupMessage("rankedBeliefR: ranked_belief build output not found. Run `make build` in the repository root before using the R package.")
  }
}
