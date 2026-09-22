package com.example.myapplication

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

data class ScoreRecord(
    val id: Long,
    val playerName: String,
    val playerScore: Int,
    val opponentScore: Int,
    val playerWon: Boolean,
    val playedAt: Long
)

/** Persistent local database for completed Pong matches. */
class ScoreDatabase(context: Context) : SQLiteOpenHelper(context, DATABASE_NAME, null, DATABASE_VERSION) {
    override fun onCreate(database: SQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE $TABLE_SCORES (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                player_name TEXT NOT NULL,
                player_score INTEGER NOT NULL,
                opponent_score INTEGER NOT NULL,
                player_won INTEGER NOT NULL,
                played_at INTEGER NOT NULL
            )
            """.trimIndent()
        )
    }

    override fun onUpgrade(database: SQLiteDatabase, oldVersion: Int, newVersion: Int) = Unit

    fun saveScore(playerName: String, playerScore: Int, opponentScore: Int, playerWon: Boolean) {
        writableDatabase.insert(
            TABLE_SCORES,
            null,
            ContentValues().apply {
                put("player_name", playerName)
                put("player_score", playerScore)
                put("opponent_score", opponentScore)
                put("player_won", if (playerWon) 1 else 0)
                put("played_at", System.currentTimeMillis())
            }
        )
    }

    fun getScores(): List<ScoreRecord> = readableDatabase.query(
        TABLE_SCORES,
        arrayOf("id", "player_name", "player_score", "opponent_score", "player_won", "played_at"),
        null,
        null,
        null,
        null,
        "played_at DESC"
    ).use { cursor ->
        buildList {
            while (cursor.moveToNext()) {
                add(
                    ScoreRecord(
                        id = cursor.getLong(0),
                        playerName = cursor.getString(1),
                        playerScore = cursor.getInt(2),
                        opponentScore = cursor.getInt(3),
                        playerWon = cursor.getInt(4) == 1,
                        playedAt = cursor.getLong(5)
                    )
                )
            }
        }
    }

    companion object {
        private const val DATABASE_NAME = "pong_scores.db"
        private const val DATABASE_VERSION = 1
        private const val TABLE_SCORES = "scores"
    }
}
