/**
 * This file is part of the CernVM file system.
 */

#include "catalog_mgr_ro.h"

#include "compression.h"
#include "download.h"
#include "util.h"
#include "pathspec/pathspec.h"

using namespace std;  // NOLINT

namespace catalog {

/**
 * Loads a catalog via HTTP from Statum 0 into a temporary file.
 * @param url_path the url of the catalog to load
 * @param mount_point the file system path where the catalog should be mounted
 * @param catalog_file a pointer to the string containing the full qualified
 *                     name of the catalog afterwards
 * @return 0 on success, different otherwise
 */
LoadError SimpleCatalogManager::LoadCatalog(const PathString  &mountpoint,
                                            const shash::Any  &hash,
                                            std::string       *catalog_path,
                                            shash::Any        *catalog_hash)
{
  shash::Any effective_hash = hash.IsNull() ? base_hash_ : hash;
  const string url = stratum0_ + "/data" + effective_hash.MakePath(1, 2) + "C";
  FILE *fcatalog = CreateTempFile(dir_temp_ + "/catalog", 0666, "w",
                                  catalog_path);
  if (!fcatalog) {
    LogCvmfs(kLogCatalog, kLogStderr,
             "failed to create temp file when loading %s", url.c_str());
    assert(false);
  }

  download::JobInfo download_catalog(&url, true, false, fcatalog,
                                     &effective_hash);

  download::Failures retval = download_manager_->Fetch(&download_catalog);
  fclose(fcatalog);

  if (retval != download::kFailOk) {
    LogCvmfs(kLogCatalog, kLogStderr,
             "failed to load %s from Stratum 0 (%d - %s)", url.c_str(),
             retval, download::Code2Ascii(retval));
    assert(false);
  }

  *catalog_hash = effective_hash;
  return kLoadNew;
}


Catalog* SimpleCatalogManager::CreateCatalog(const PathString  &mountpoint,
                                             const shash::Any  &catalog_hash,
                                             Catalog           *parent_catalog) {
  return new Catalog(mountpoint, catalog_hash, parent_catalog);
}


struct ParentInfo {
  ParentInfo(const PathString &path, const bool is_mnt_pnt) :
    path(path), is_nested_mountpoint(is_mnt_pnt) {}
  PathString  path;
  bool        is_nested_mountpoint;
};
typedef std::vector<ParentInfo> ParentInfos;

/**
 * Collect all paths that satisfy the given Pathspec
 * @param pathspec  the pathspec to be used for the lookup
 * @param listing   the result vector for the listing
 * @return          true if listing succeeded
 */
bool SimpleCatalogManager::LookupPathspec(const Pathspec             &pathspec,
                                          LocatedDirectoryEntryList  *listing) {
  EnforceSqliteMemLimit();

  // for relative pathspecs we do not have any sensible anchor point. Thus we
  // prohibit them upfront
  if (!pathspec.IsAbsolute()) {
    LogCvmfs(kLogCatalog, kLogVerboseMsg, "non-absolute pathspec lookup "
                                          "is not supported");
    return false;
  }

  // generate a glob string sequence from the Pathspec
  // each sequence entry describes _one_ directory level
  const Pathspec::GlobStringSequence& gss = pathspec.GetGlobStringSequence();
  unsigned int gss_stage = 1;
  LogCvmfs(kLogCatalog, kLogDebug, "doing %d-stage pathspec lookup",
           gss.size());

  // set up the 'recursion' state. Directories found in pathspec stage N are
  // used as parent directories in the lookup of stage N + 1
  // Note: as initial value we take the root entry of the repository
  ParentInfos parent_paths;
  parent_paths.push_back(ParentInfo(PathString(""), false));

  ReadLock();

  // go through all the pathspec stages
        Pathspec::GlobStringSequence::const_iterator g    = gss.begin();
  const Pathspec::GlobStringSequence::const_iterator gend = gss.end();
  for (; g != gend; ++g, ++gss_stage) {
    unsigned int matches_found = 0;
    const bool   last_stage    = (gss_stage == gss.size());
    ParentInfos  next_parent_paths;

    // go through all the directories matched in the previous pathspec stage
          ParentInfos::const_iterator p    = parent_paths.begin();
    const ParentInfos::const_iterator pend = parent_paths.end();
    for (; p != pend; ++p) {
      // acquire the catalog for the globbed lookup
      const ParentInfo &parent = *p;
      Catalog *clg = FindCatalog(parent.path);
      if (clg == NULL) {
        LogCvmfs(kLogCatalog, kLogDebug, "Failed to locate catalog for parent"
                                         "directory '%s'",
                                         parent.path.c_str());
        return false;
      }

      // we hit a nested catalog mountpoint but the nested catalog seems not
      // to be loaded yet... go ahead and try to mount it
      if (parent.is_nested_mountpoint && parent.path != clg->path()) {
        LogCvmfs(kLogCatalog, kLogDebug, "Loading nested catalog for '%s'",
                 parent.path.c_str());
        Unlock();
        WriteLock();
        // check again to avoid race
        Catalog *new_clg = FindCatalog(parent.path);
        if (new_clg == clg) {
          new_clg = NULL;
          if (! MountSubtree(parent.path, clg, &new_clg) || new_clg == NULL) {
            LogCvmfs(kLogCatalog, kLogDebug, "Failed to load nested catalog "
                                             " for '%s'",
                                             parent.path.c_str());
            return false;
          }
        }
        assert (new_clg != clg);
        clg = new_clg;
      }

      // do the actual globbed lookup for the current pathspec stage
      DirectoryEntryList matching_entries;
      if (! clg->LookupGlobString(*g, parent.path, &matching_entries)) {
        LogCvmfs(kLogCatalog, kLogDebug, "glob-lookup '%s' with parent path "
                                         "'%s' in catalog '%s' failed",
                                         g->c_str(), parent.path.c_str(),
                                         clg->path().c_str());
        return false;
      }

      matches_found += matching_entries.size();

      // filter out directories from the result of the globbed lookup to be
      // used as parent directories in the next pathspec stage
            DirectoryEntryList::const_iterator d    = matching_entries.begin();
      const DirectoryEntryList::const_iterator dend = matching_entries.end();
      for (; d != dend; ++d) {
        // generate absolute directory entry path
        PathString entry_path = parent.path;
        entry_path.Append("/", 1); entry_path.Append(d->name());

        // directories will be used in the next pathspec stage
        if (d->IsDirectory()) {
          const ParentInfo npi(entry_path, d->IsNestedCatalogMountpoint());
          next_parent_paths.push_back(npi);
        }

        // if we hit the last stage, we are actually looking at the final
        // result directory entries and should write them to the provided
        // result list
        if (last_stage) {
          listing->push_back(LocatedDirectoryEntry(*d, entry_path));
        }
      }
    }

    // no matches found in a particular pathspec stage --> stop processing
    // otherwise update the parent paths for the next stage and continue
    if (matches_found == 0) {
      LogCvmfs(kLogCatalog, kLogDebug, "Failed to locate any matching entry "
                                       "for pathspec stage number %d (%s).",
                                       gss_stage, g->c_str());
      return true; // lookup didn't fail (just didn't produce any matches)
    } else {
      LogCvmfs(kLogCatalog, kLogDebug, "Found %d entries (%d directories) for "
                                       "pathspec stage number %d (%s)",
                                       matches_found, next_parent_paths.size(),
                                       gss_stage, g->c_str());
      parent_paths = next_parent_paths;
    }
  }

  Unlock();
  return true;
}


} // namespace catalog
