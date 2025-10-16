package com.godot.game

import android.content.Intent
import android.net.Uri
import android.util.Log
import android.widget.Toast
import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.plugin.UsedByGodot


class DclAndroidPlugin(godot: Godot) : GodotPlugin(godot) {
    override fun getPluginName(): String {
        return "DclAndroidPlugin"
    }

    /**
     * Example showing how to declare a method that's used by Godot.
     *
     * Shows a 'Decentraland Mobile' toast.
     */
    @UsedByGodot
    private fun showDecentralandMobileToast() {
        runOnUiThread {
            Toast.makeText(activity, "Decentraland Mobile", Toast.LENGTH_LONG).show()
            Log.v(pluginName, "Decentraland Mobile")
        }
    }

    /**
     * Open an URL in a integrated browser
     */
    @UsedByGodot
    private fun openUrl(url: String) {
        runOnUiThread {
            val i = Intent(Intent.ACTION_VIEW)
            i.data = Uri.parse(url)
            godot.getActivity()?.startActivity(i)
        }
    }
}