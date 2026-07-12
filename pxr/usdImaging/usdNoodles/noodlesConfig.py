#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
NoodlesConfig - Unified configuration access layer for the usdNoodles editor.

This module provides a singleton configuration manager that handles priority/override
logic between disk-based settings and USD-embedded configuration.

Priority hierarchy (highest to lowest):
1. USD-embedded config (per-file overrides from customData)
2. Disk-based config (~/.usdview/state.json)
3. Hardcoded defaults (in NoodlesSettingsDataModel)

Usage:
    # Initialize during GraphView setup
    from .noodlesConfig import NoodlesConfig
    NoodlesConfig.initialize(settingsObject, graphView=self)

    # Load USD overrides when opening a graph
    NoodlesConfig.loadFromUsd(stage, primPath)

    # Access config values anywhere
    fontSize = NoodlesConfig.get("nodeTitleFontSize", 24.0)
    linkWidth = NoodlesConfig.get("linkLineWidth", 10.0)

    # Save settings
    NoodlesConfig.save()
"""

from pxr import Tf

from .noodlesSettings import NoodlesSettingsDataModel, NoodlesUIState


class NoodlesConfig:
    """
    Singleton configuration manager that handles priority/override logic.
    Provides simple API for accessing settings throughout the editor.

    Priority hierarchy (highest to lowest):
    1. USD-embedded config (per-file overrides)
    2. Disk-based config (~/.usdview/state.json)
    3. Hardcoded defaults (in NoodlesSettingsDataModel)
    """

    _instance = None
    _settings = None  # NoodlesSettingsDataModel (from disk/defaults)
    _uiState = None  # NoodlesUIState (UI layout state)
    _usdOverrides = None  # Dict of USD-embedded overrides

    @classmethod
    def initialize(cls, settingsObject, graphView=None):
        """
        Initialize the config system.
        Called during GraphView initialization.

        Args:
            settingsObject: Settings instance from usdviewq's ConfigManager
            graphView: GraphView instance for UI state tracking (optional)
        """
        if cls._instance is None:
            cls._instance = cls()

        # Create or attach to NoodlesSettingsDataModel
        noodlesSettings = settingsObject.GetChildStateSource("noodles")
        if noodlesSettings is None:
            cls._settings = NoodlesSettingsDataModel(settingsObject)
        else:
            cls._settings = noodlesSettings

        # Create UI state tracker if graphView provided
        if graphView:
            uiState = settingsObject.GetChildStateSource("ui")
            if uiState is None:
                cls._uiState = NoodlesUIState(graphView, settingsObject)
            else:
                cls._uiState = uiState

        cls._usdOverrides = {}

    @classmethod
    def loadFromUsd(cls, stage, primPath):
        """
        Load configuration overrides from USD customData.

        Looks for customData at:
        1. Blueprint prim's customData["noodlesConfig"]
        2. Special /NoodlesConfig prim's customData

        Example USD customData:
            def Blueprint "MyGraph" (
                customData = {
                    dictionary noodlesConfig = {
                        float nodeTitleFontSize = 28.0
                        float[] backgroundClearColor = [0.1, 0.1, 0.15, 1.0]
                        float linkLineWidth = 12.0
                    }
                }
            )

        Args:
            stage: USD stage containing the graph
            primPath: Path to the Blueprint prim
        """
        cls._usdOverrides = {}

        # Try loading from Blueprint prim
        blueprintPrim = stage.GetPrimAtPath(primPath)
        if blueprintPrim and blueprintPrim.IsValid():
            customData = blueprintPrim.GetCustomData()
            if "noodlesConfig" in customData:
                cls._usdOverrides = dict(customData["noodlesConfig"])
                Tf.Status(
                    f"Loaded {len(cls._usdOverrides)} config overrides from {primPath}"
                )
                return

        # Fall back to special config prim
        configPrim = stage.GetPrimAtPath("/NoodlesConfig")
        if configPrim and configPrim.IsValid():
            customData = configPrim.GetCustomData()
            if "settings" in customData:
                cls._usdOverrides = dict(customData["settings"])
                Tf.Status(
                    f"Loaded {len(cls._usdOverrides)} config overrides from /NoodlesConfig prim"
                )

    @classmethod
    def clearUsdOverrides(cls):
        """
        Clear all USD-embedded config overrides.

        Call this when closing a graph or switching to a different graph
        to ensure overrides don't bleed between files.
        """
        cls._usdOverrides = {}

    @classmethod
    def get(cls, key, default=None):
        """
        Get a configuration value with priority handling.

        Priority: USD overrides > disk settings > provided default

        Args:
            key: Setting key (e.g., "nodeTitleFontSize")
            default: Fallback if not found anywhere

        Returns:
            Configuration value
        """
        # Highest priority: USD overrides
        if cls._usdOverrides and key in cls._usdOverrides:
            return cls._usdOverrides[key]

        # Medium priority: Disk-based settings
        if cls._settings and hasattr(cls._settings, key):
            return getattr(cls._settings, key)

        # Lowest priority: Provided default
        return default

    @classmethod
    def set(cls, key, value):
        """
        Set a configuration value in the disk-based settings.

        Note: This does not affect USD overrides. To persist changes,
        call save() after setting values.

        Args:
            key: Setting key (e.g., "nodeTitleFontSize")
            value: New value for the setting
        """
        if cls._settings and hasattr(cls._settings, key):
            setattr(cls._settings, key, value)
        else:
            Tf.Warn(f"Attempted to set unknown config key: {key}")

    @classmethod
    def settings(cls):
        """
        Get direct access to NoodlesSettingsDataModel for advanced use.

        Returns:
            NoodlesSettingsDataModel instance or None if not initialized
        """
        return cls._settings

    @classmethod
    def uiState(cls):
        """
        Get direct access to NoodlesUIState for UI state management.

        Returns:
            NoodlesUIState instance or None if not initialized
        """
        return cls._uiState

    @classmethod
    def save(cls):
        """
        Save current settings to disk via Settings.save().

        This persists all changes made to disk-based settings to
        ~/.usdview/state.json.
        """
        if cls._settings and cls._settings._parentStateSource:
            cls._settings._parentStateSource.save()

    @classmethod
    def getUsdOverrides(cls):
        """
        Get the current USD override dictionary.

        Useful for debugging or displaying which settings are being
        overridden by the current USD file.

        Returns:
            Dictionary of USD override key-value pairs
        """
        return dict(cls._usdOverrides) if cls._usdOverrides else {}

    @classmethod
    def hasUsdOverride(cls, key):
        """
        Check if a setting has a USD override active.

        Args:
            key: Setting key to check

        Returns:
            True if the key has a USD override, False otherwise
        """
        return cls._usdOverrides and key in cls._usdOverrides

    @classmethod
    def isInitialized(cls):
        """
        Check if the config system has been initialized.

        Returns:
            True if initialized, False otherwise
        """
        return cls._settings is not None
