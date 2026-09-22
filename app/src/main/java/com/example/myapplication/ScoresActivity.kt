package com.example.myapplication

import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.text.DateFormat
import java.util.Date

/** Separate screen that lists all completed matches saved in the local SQLite database. */
class ScoresActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        showScores()
    }

    private fun showScores() {
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 42, 48, 42)
            setBackgroundColor(Color.rgb(18, 8, 31))
        }
        content.addView(TextView(this).apply {
            text = "PUNTAJES"
            setTextColor(Color.WHITE)
            textSize = 30f
            gravity = Gravity.CENTER
        })

        val scores = ScoreDatabase(this).getScores()
        if (scores.isEmpty()) {
            content.addView(TextView(this).apply {
                text = "Todavía no hay partidas guardadas."
                setTextColor(Color.LTGRAY)
                textSize = 18f
                setPadding(0, 48, 0, 24)
                gravity = Gravity.CENTER
            })
        } else {
            scores.forEach { score ->
                content.addView(TextView(this).apply {
                    val result = if (score.playerWon) "GANÓ" else "PERDIÓ"
                    val date = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT)
                        .format(Date(score.playedAt))
                    text = "${score.playerName}   ${score.playerScore} - ${score.opponentScore}   $result\n$date"
                    setTextColor(if (score.playerWon) Color.rgb(109, 255, 180) else Color.rgb(255, 155, 155))
                    textSize = 18f
                    setPadding(22, 24, 22, 24)
                })
            }
        }

        content.addView(Button(this).apply {
            text = "VOLVER"
            setOnClickListener { finish() }
        })
        setContentView(ScrollView(this).apply { addView(content) })
    }
}
