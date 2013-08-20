# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import os

from branch_utility import BranchUtility
from compiled_file_system import CompiledFileSystem
from file_system import FileNotFoundError
import svn_constants
from third_party.json_schema_compiler import json_parse, model
from third_party.json_schema_compiler.memoize import memoize

_API_AVAILABILITIES = svn_constants.JSON_PATH + '/api_availabilities.json'
_API_FEATURES = svn_constants.API_PATH + '/_api_features.json'
_EXTENSION_API = svn_constants.API_PATH + '/extension_api.json'
_MANIFEST_FEATURES = svn_constants.API_PATH + '/_manifest_features.json'
_PERMISSION_FEATURES = svn_constants.API_PATH + '/_permission_features.json'
_STABLE = 'stable'

class AvailabilityInfo(object):
  def __init__(self, channel, version):
    self.channel = channel
    self.version = version

def _GetChannelFromFeatures(api_name, file_system, path):
  '''Finds API channel information within _features.json files at the given
  |path| for the given |file_system|. Returns None if channel information for
  the API cannot be located.
  '''
  feature = file_system.GetFromFile(path).get(api_name)

  if feature is None:
    return None
  if isinstance(feature, collections.Mapping):
    # The channel information exists as a solitary dict.
    return feature.get('channel')
  # The channel information dict is nested within a list for whitelisting
  # purposes. Take the newest channel out of all of the entries.
  return BranchUtility.NewestChannel(entry.get('channel') for entry in feature)

def _GetChannelFromApiFeatures(api_name, file_system):
  try:
    return _GetChannelFromFeatures(api_name, file_system, _API_FEATURES)
  except FileNotFoundError:
    # TODO(epeterson) Remove except block once _api_features is in all channels.
    return None

def _GetChannelFromPermissionFeatures(api_name, file_system):
  return _GetChannelFromFeatures(api_name, file_system, _PERMISSION_FEATURES)

def _GetChannelFromManifestFeatures(api_name, file_system):
  return _GetChannelFromFeatures(#_manifest_features uses unix_style API names
                                 model.UnixName(api_name),
                                 file_system,
                                 _MANIFEST_FEATURES)

def _ExistsInFileSystem(api_name, file_system):
  '''Checks for existence of |api_name| within the list of files in the api/
  directory found using the given file system.
  '''
  file_names = file_system.GetFromFileListing(svn_constants.API_PATH)
  # File names switch from unix_hacker_style to camelCase at versions <= 20.
  return model.UnixName(api_name) in file_names or api_name in file_names

def _ExistsInExtensionApi(api_name, file_system):
  '''Parses the api/extension_api.json file (available in Chrome versions
  before 18) for an API namespace. If this is successfully found, then the API
  is considered to have been 'stable' for the given version.
  '''
  try:
    extension_api_json = file_system.GetFromFile(_EXTENSION_API)
    api_rows = [row.get('namespace') for row in extension_api_json
                if 'namespace' in row]
    return True if api_name in api_rows else False
  except FileNotFoundError:
    # This should only happen on preview.py since extension_api.json is no
    # longer present in trunk.
    return False

class AvailabilityFinder(object):
  '''Uses API data sources generated by a ChromeVersionDataSource in order to
  search the filesystem for the earliest existence of a specified API throughout
  the different versions of Chrome; this constitutes an API's availability.
  '''
  class Factory(object):
    def __init__(self,
                 object_store_creator,
                 compiled_host_fs_factory,
                 branch_utility,
                 host_file_system_creator):
      self._object_store_creator = object_store_creator
      self._compiled_host_fs_factory = compiled_host_fs_factory
      self._branch_utility = branch_utility
      self._host_file_system_creator = host_file_system_creator

    def Create(self):
      return AvailabilityFinder(self._object_store_creator,
                                self._compiled_host_fs_factory,
                                self._branch_utility,
                                self._host_file_system_creator)

  def __init__(self,
               object_store_creator,
               compiled_host_fs_factory,
               branch_utility,
               host_file_system_creator):
    self._object_store_creator = object_store_creator
    self._json_cache = compiled_host_fs_factory.Create(
        lambda _, json: json_parse.Parse(json),
        AvailabilityFinder,
        'json-cache')
    self._branch_utility = branch_utility
    self._host_file_system_creator = host_file_system_creator
    self._object_store = object_store_creator.Create(AvailabilityFinder)

  @memoize
  def _CreateFeaturesAndNamesFileSystems(self, version):
    '''The 'features' compiled file system's populate function parses and
    returns the contents of a _features.json file. The 'names' compiled file
    system's populate function creates a list of file names with .json or .idl
    extensions.
    '''
    fs_factory = CompiledFileSystem.Factory(
        self._host_file_system_creator.Create(
            self._branch_utility.GetBranchForVersion(version)),
        self._object_store_creator)
    features_fs = fs_factory.Create(lambda _, json: json_parse.Parse(json),
                                    AvailabilityFinder,
                                    category='features')
    names_fs = fs_factory.Create(self._GetExtNames,
                                 AvailabilityFinder,
                                 category='names')
    return (features_fs, names_fs)

  def _GetExtNames(self, base_path, apis):
    return [os.path.splitext(api)[0] for api in apis
            if os.path.splitext(api)[1][1:] in ['json', 'idl']]

  def _FindEarliestStableAvailability(self, api_name, version):
    '''Searches in descending order through filesystem caches tied to specific
    chrome version numbers and looks for the availability of an API, |api_name|,
    on the stable channel. When a version is found where the API is no longer
    available on stable, returns the previous version number (the last known
    version where the API was stable).
    '''
    available = True
    while available:
      if version < 5:
        # SVN data isn't available below version 5.
        return version + 1
      available = False
      available_channel = None
      features_fs, names_fs = self._CreateFeaturesAndNamesFileSystems(version)
      if version >= 28:
        # The _api_features.json file first appears in version 28 and should be
        # the most reliable for finding API availabilities, so it gets checked
        # first. The _permission_features.json and _manifest_features.json files
        # are present in Chrome 20 and onwards. Fall back to a check for file
        # system existence if the API is not stable in any of the _features.json
        # files.
        available_channel = _GetChannelFromApiFeatures(api_name, features_fs)
      if version >= 20:
        # Check other _features.json files/file existence if the API wasn't
        # found in _api_features.json, or if _api_features.json wasn't present.
        available_channel = available_channel or (
            _GetChannelFromPermissionFeatures(api_name, features_fs)
            or _GetChannelFromManifestFeatures(api_name, features_fs))
        if available_channel is None:
          available = _ExistsInFileSystem(api_name, names_fs)
        else:
          available = available_channel == _STABLE
      elif version >= 18:
        # These versions are a little troublesome. Version 19 has
        # _permission_features.json, but it lacks 'channel' information.
        # Version 18 lacks all of the _features.json files. For now, we're using
        # a simple check for filesystem existence here.
        available = _ExistsInFileSystem(api_name, names_fs)
      elif version >= 5:
        # Versions 17 and down to 5 have an extension_api.json file which
        # contains namespaces for each API that was available at the time. We
        # can use this file to check for API existence.
        available = _ExistsInExtensionApi(api_name, features_fs)

      if not available:
        return version + 1
      version -= 1

  def _GetAvailableChannelForVersion(self, api_name, version):
    '''Searches through the _features files for a given |version| and returns
    the channel that the given API is determined to be available on.
    '''
    features_fs, names_fs = self._CreateFeaturesAndNamesFileSystems(version)
    available_channel = (_GetChannelFromApiFeatures(api_name, features_fs)
        or _GetChannelFromPermissionFeatures(api_name, features_fs)
        or _GetChannelFromManifestFeatures(api_name, features_fs))
    if available_channel is None and _ExistsInFileSystem(api_name, names_fs):
      # If an API is not represented in any of the _features files, but exists
      # in the filesystem, then assume it is available in this version.
      # The windows API is an example of this.
      return self._branch_utility.GetChannelForVersion(version)

    return available_channel

  def GetApiAvailability(self, api_name):
    '''Determines the availability for an API by testing several scenarios.
    (i.e. Is the API experimental? Only available on certain development
    channels? If it's stable, when did it first become stable? etc.)
    '''
    availability = self._object_store.Get(api_name).Get()
    if availability is not None:
      return availability

    # Check for a predetermined availability for this API.
    api_info = self._json_cache.GetFromFile(_API_AVAILABILITIES).get(api_name)
    if api_info is not None:
      channel = api_info.get('channel')
      if channel == _STABLE:
        version = api_info.get('version')
      else:
        version = self._branch_utility.GetChannelInfo(channel).version
      # The file data for predetermined availabilities is already cached, so
      # skip caching this result.
      return AvailabilityInfo(channel, version)

    # Check for the API in the development channels.
    availability = None

    for channel_info in self._branch_utility.GetAllChannelInfo():
      if channel_info.channel == 'trunk':
        # Don't check trunk, since (a) there's no point, we know it's going to
        # be available there, and (b) there is a bug with the current
        # architecture and design of HostFileSystemCreator, where creating
        # 'trunk' ignores the pinned revision (in fact, it bypasses every
        # difference including whether the file system is patched).
        # TODO(kalman): Fix HostFileSystemCreator and update this comment.
        break

      available_channel = self._GetAvailableChannelForVersion(
          api_name,
          channel_info.version)
      # If the |available_channel| for the API is the same as, or older than,
      # the channel we're checking, then the API is available on this channel.
      if (available_channel is not None and
          BranchUtility.NewestChannel((available_channel, channel_info.channel))
              == channel_info.channel):
        availability = AvailabilityInfo(channel_info.channel,
                                        channel_info.version)
        break

    if availability is None:
      trunk_info = self._branch_utility.GetChannelInfo('trunk')
      availability = AvailabilityInfo(trunk_info.channel, trunk_info.version)

    # If the API is in stable, find the chrome version in which it became
    # stable.
    if availability.channel == _STABLE:
      availability.version = self._FindEarliestStableAvailability(
          api_name,
          availability.version)

    self._object_store.Set(api_name, availability)
    return availability
