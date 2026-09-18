package com.example.myapplication

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.os.Bundle
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.random.Random

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).hide(WindowInsetsCompat.Type.systemBars())
        setContentView(PongView(this))
    }
}

/** A self-contained, two-player Pong implementation written entirely in Kotlin. */
private class PongView(context: Context) : View(context) {
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val pointerSides = mutableMapOf<Int, Side>()

    private enum class Side { LEFT, RIGHT }

    private var leftScore = 0
    private var rightScore = 0
    private var leftPaddleY = 0f
    private var rightPaddleY = 0f
    private var leftTargetY = 0f
    private var rightTargetY = 0f
    private var ballX = 0f
    private var ballY = 0f
    private var ballVelocityX = 0f
    private var ballVelocityY = 0f
    private var previousFrame = SystemClock.elapsedRealtimeNanos()

    private val fieldTop get() = height * 0.16f
    private val fieldBottom get() = height * 0.94f
    private val paddleWidth get() = max(18f, width * 0.018f)
    private val paddleHeight get() = max(120f, height * 0.18f)
    private val ballRadius get() = max(13f, width * 0.014f)
    private val leftPaddleX get() = width * 0.06f
    private val rightPaddleX get() = width * 0.94f

    init {
        paint.strokeCap = Paint.Cap.SQUARE
        isFocusable = true
    }

    override fun onSizeChanged(width: Int, height: Int, oldWidth: Int, oldHeight: Int) {
        super.onSizeChanged(width, height, oldWidth, oldHeight)
        leftPaddleY = height / 2f
        rightPaddleY = height / 2f
        leftTargetY = leftPaddleY
        rightTargetY = rightPaddleY
        resetBall(if (Random.nextBoolean()) 1f else -1f)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val now = SystemClock.elapsedRealtimeNanos()
        val dt = min((now - previousFrame) / 1_000_000_000f, 0.033f)
        previousFrame = now
        update(dt)

        canvas.drawColor(Color.rgb(18, 8, 31))
        drawScore(canvas)
        drawCenterLine(canvas)

        paint.style = Paint.Style.FILL
        paint.color = Color.rgb(59, 204, 255)
        drawPaddle(canvas, leftPaddleX, leftPaddleY)
        paint.color = Color.rgb(255, 79, 62)
        drawPaddle(canvas, rightPaddleX, rightPaddleY)
        paint.color = Color.YELLOW
        canvas.drawCircle(ballX, ballY, ballRadius, paint)

        postInvalidateOnAnimation()
    }

    private fun update(dt: Float) {
        val minPaddleY = fieldTop + paddleHeight / 2f
        val maxPaddleY = fieldBottom - paddleHeight / 2f
        leftPaddleY += (leftTargetY.coerceIn(minPaddleY, maxPaddleY) - leftPaddleY) * min(1f, dt * 16f)
        rightPaddleY += (rightTargetY.coerceIn(minPaddleY, maxPaddleY) - rightPaddleY) * min(1f, dt * 16f)

        ballX += ballVelocityX * dt
        ballY += ballVelocityY * dt

        if (ballY - ballRadius < fieldTop || ballY + ballRadius > fieldBottom) {
            ballY = ballY.coerceIn(fieldTop + ballRadius, fieldBottom - ballRadius)
            ballVelocityY = -ballVelocityY
        }

        if (ballVelocityX < 0 && hitsPaddle(leftPaddleX, leftPaddleY)) {
            bounceFrom(leftPaddleY, 1f)
        } else if (ballVelocityX > 0 && hitsPaddle(rightPaddleX, rightPaddleY)) {
            bounceFrom(rightPaddleY, -1f)
        }

        if (ballX + ballRadius < 0) {
            rightScore++
            resetBall(1f)
        } else if (ballX - ballRadius > width) {
            leftScore++
            resetBall(-1f)
        }
    }

    private fun hitsPaddle(paddleX: Float, paddleY: Float): Boolean {
        return abs(ballX - paddleX) < paddleWidth / 2f + ballRadius &&
            abs(ballY - paddleY) < paddleHeight / 2f + ballRadius
    }

    private fun bounceFrom(paddleY: Float, direction: Float) {
        ballX = if (direction > 0) leftPaddleX + paddleWidth / 2f + ballRadius else rightPaddleX - paddleWidth / 2f - ballRadius
        val hitOffset = ((ballY - paddleY) / (paddleHeight / 2f)).coerceIn(-1f, 1f)
        val speed = min(1_250f, max(700f, abs(ballVelocityX) * 1.04f))
        ballVelocityX = speed * direction
        ballVelocityY = hitOffset * speed * 0.75f
    }

    private fun resetBall(direction: Float) {
        ballX = width / 2f
        ballY = (fieldTop + fieldBottom) / 2f
        ballVelocityX = 700f * direction
        ballVelocityY = Random.nextFloat() * 420f - 210f
    }

    private fun drawPaddle(canvas: Canvas, x: Float, y: Float) {
        canvas.drawRect(x - paddleWidth / 2f, y - paddleHeight / 2f, x + paddleWidth / 2f, y + paddleHeight / 2f, paint)
    }

    private fun drawScore(canvas: Canvas) {
        paint.color = Color.rgb(240, 246, 255)
        paint.typeface = Typeface.create(Typeface.MONOSPACE, Typeface.BOLD)
        paint.textAlign = Paint.Align.CENTER
        paint.textSize = max(54f, width * 0.08f)
        canvas.drawText("$leftScore  $rightScore", width / 2f, height * 0.105f, paint)
    }

    private fun drawCenterLine(canvas: Canvas) {
        paint.color = Color.argb(110, 255, 255, 255)
        paint.strokeWidth = max(5f, width * 0.006f)
        var y = fieldTop + 22f
        while (y < fieldBottom) {
            canvas.drawLine(width / 2f, y, width / 2f, min(y + 28f, fieldBottom), paint)
            y += 52f
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val index = event.actionIndex
                pointerSides[event.getPointerId(index)] = if (event.getX(index) < width / 2f) Side.LEFT else Side.RIGHT
                movePaddle(event.getPointerId(index), event.getY(index))
            }
            MotionEvent.ACTION_MOVE -> {
                for (index in 0 until event.pointerCount) movePaddle(event.getPointerId(index), event.getY(index))
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                pointerSides.remove(event.getPointerId(event.actionIndex))
            }
        }
        return true
    }

    private fun movePaddle(pointerId: Int, y: Float) {
        when (pointerSides[pointerId]) {
            Side.LEFT -> leftTargetY = y
            Side.RIGHT -> rightTargetY = y
            null -> Unit
        }
    }
}
