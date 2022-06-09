$$
\text{F}0_{长度} = \frac{x_{音频长度} \cdot 1000}{T_{帧周期} \cdot x_{采样率}} + 1
$$

```c++
1000.0 * x_length / fs / frame_period + 1;
```


