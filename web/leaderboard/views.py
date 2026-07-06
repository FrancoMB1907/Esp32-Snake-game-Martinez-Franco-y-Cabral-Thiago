from django.shortcuts import render
from .models import Score


def home(request):
    return render(request, 'leaderboard/home.html')


def leaderboard(request):
    scores = Score.objects.all()
    difficulty = request.GET.get('difficulty', '')
    player = request.GET.get('player', '')

    if difficulty:
        scores = scores.filter(difficulty=difficulty)
    if player:
        scores = scores.filter(player_name__icontains=player)

    difficulties = Score.objects.values_list('difficulty', flat=True).distinct()
    return render(request, 'leaderboard/leaderboard.html', {
        'scores': scores,
        'difficulties': difficulties,
        'current_difficulty': difficulty,
        'current_player': player,
    })
